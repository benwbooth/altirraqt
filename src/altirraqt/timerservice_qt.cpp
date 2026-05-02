//	Altirra - Qt port
//	Real implementation of IATTimerService backed by QTimer.
//
//	The interface (src/h/at/atcore/timerservice.h) lets devices queue a
//	callback to fire approximately N seconds in real time, with token-based
//	cancellation. Upstream's Win32 impl uses a thread-pool timer that wakes
//	the IATAsyncDispatcher; the dispatcher then runs callbacks on the
//	emulation thread.
//
//	In altirraqt the simulator runs on the Qt main thread (driven by a
//	zero-interval QTimer in main.cpp's event loop). QTimer slots also fire on
//	that same main thread, so we don't need IATAsyncDispatcher to marshal
//	threads — we can invoke callbacks directly from QTimer's `timeout` slot
//	and still satisfy the upstream contract that callbacks run on the
//	emulation thread. The IATAsyncDispatcher reference is accepted to keep
//	the constructor signature compatible, but is unused.
//
//	The interface promises:
//	  - Cancel() on the dispatching thread guarantees the callback won't
//	    fire afterwards. Since QTimer::stop() before the next event-loop
//	    iteration prevents `timeout` from firing, this holds on the main
//	    thread.
//	  - A device may call Cancel() (or Request() with the same token) from
//	    inside its own callback. We handle this by tracking the currently
//	    firing slot and using deleteLater() so the QTimer destruction never
//	    races against its own slot.
//	  - Callbacks coalesce at ~50 ms upstream; QTimer has ~1 ms host
//	    resolution which is well below that, so we just clamp to a 1 ms
//	    floor.

#include <algorithm>
#include <cstdint>
#include <utility>
#include <vector>

#include <QObject>
#include <QPointer>
#include <QTimer>

#include <vd2/system/function.h>

#include <at/atcore/asyncdispatcher.h>
#include <at/atcore/timerservice.h>

namespace {

class ATQtTimerService final : public QObject, public IATTimerService {
public:
	explicit ATQtTimerService(IATAsyncDispatcher&) {}
	~ATQtTimerService() override {
		// Stop and delete every outstanding QTimer. We can't deleteLater()
		// here because our parent (this) is being destroyed; just delete.
		for (Slot& slot : mSlots) {
			if (slot.mTimer) {
				slot.mTimer->stop();
				delete slot.mTimer;
				slot.mTimer = nullptr;
			}
			slot.mCallback = nullptr;
		}
	}

	using Callback = vdfunction<void()>;

	void Request(uint64 *token, float delay, Callback fn) override {
		if (!fn)
			return;

		// If the caller passed an existing token, cancel its current binding
		// before reissuing. This matches upstream semantics where Request()
		// on a live token replaces the previous queued callback.
		if (token)
			Cancel(token);

		// Clamp to upstream's documented 0..60 s range, with a 1 ms floor so
		// we don't degenerate into "fire on the next tick" forever (Qt would
		// happily accept 0 but devices expecting a real delay shouldn't get
		// instant fire).
		const float clamped = std::clamp(delay, 0.0f, 60.0f);
		int delayMs = (int)(clamped * 1000.0f + 0.5f);
		if (delayMs < 1) delayMs = 1;

		// Allocate a slot.
		uint32 slotIdx;
		if (!mFreeSlots.empty()) {
			slotIdx = mFreeSlots.back();
			mFreeSlots.pop_back();
		} else {
			slotIdx = (uint32)mSlots.size();
			mSlots.emplace_back();
		}

		Slot& slot = mSlots[slotIdx];
		++slot.mSequenceNo;
		slot.mActive = true;
		slot.mCallback = std::move(fn);

		// Bind the QTimer to this slot's sequence number; on fire we
		// re-check it to confirm the slot wasn't repurposed mid-flight.
		QTimer *timer = new QTimer(this);
		timer->setSingleShot(true);
		const uint32 seq = slot.mSequenceNo;
		QObject::connect(timer, &QTimer::timeout, this, [this, slotIdx, seq]() {
			OnTimerFired(slotIdx, seq);
		});
		slot.mTimer = timer;
		timer->start(delayMs);

		if (token)
			*token = ((uint64)slot.mSequenceNo << 32) | ((uint64)slotIdx + 1);
	}

	void Cancel(uint64 *tokenPtr) override {
		if (!tokenPtr) return;
		const uint64 token = *tokenPtr;
		*tokenPtr = 0;
		if (!token) return;

		const uint32 slotIdx = (uint32)((token & 0xFFFFFFFFu) - 1);
		const uint32 seq     = (uint32)(token >> 32);
		if (slotIdx >= mSlots.size()) return;

		Slot& slot = mSlots[slotIdx];
		if (!slot.mActive || slot.mSequenceNo != seq) return;

		// Bump sequence so any stale token lookup misses, and so the
		// in-flight QTimer slot (if it somehow survived stop()) sees the
		// mismatch and bails.
		++slot.mSequenceNo;
		slot.mActive = false;
		slot.mCallback = nullptr;

		// If we're inside this slot's own callback, OnTimerFired will tear
		// down the QTimer when it returns. Otherwise free it now.
		if (mFiringSlot != (sint64)slotIdx) {
			TearDownSlot(slot, slotIdx);
		}
	}

private:
	struct Slot {
		QTimer *mTimer = nullptr;
		Callback mCallback;
		uint32 mSequenceNo = 0;
		bool mActive = false;
	};

	void OnTimerFired(uint32 slotIdx, uint32 seq) {
		if (slotIdx >= mSlots.size()) return;
		Slot& slot = mSlots[slotIdx];
		if (!slot.mActive || slot.mSequenceNo != seq) return;

		// Move the callback out so a Cancel/Request from inside the
		// callback can repopulate the slot freely without us holding a
		// stale copy.
		Callback cb = std::move(slot.mCallback);
		slot.mCallback = nullptr;

		// Mark slot as no-longer-active before firing — the firing window
		// guards against re-entry deleting our QTimer mid-callback.
		slot.mActive = false;
		++slot.mSequenceNo;

		const sint64 prevFiring = mFiringSlot;
		mFiringSlot = (sint64)slotIdx;

		if (cb) cb();

		mFiringSlot = prevFiring;

		// If the callback rebound this slot via Request(), it now has a
		// fresh QTimer + active=true; don't tear that one down.
		if (!slot.mActive)
			TearDownSlot(slot, slotIdx);
	}

	void TearDownSlot(Slot& slot, uint32 slotIdx) {
		if (slot.mTimer) {
			slot.mTimer->stop();
			// deleteLater() defers destruction past the current event loop
			// iteration, which is essential when this runs from inside the
			// QTimer's own timeout handler (Qt forbids deleting a QObject
			// while one of its own slots is on the stack).
			slot.mTimer->deleteLater();
			slot.mTimer = nullptr;
		}
		slot.mCallback = nullptr;
		mFreeSlots.push_back(slotIdx);
	}

	std::vector<Slot>     mSlots;
	std::vector<uint32>   mFreeSlots;
	sint64                mFiringSlot = -1;
};

} // namespace

IATTimerService *ATCreateTimerService(IATAsyncDispatcher& dispatcher) {
	return new ATQtTimerService(dispatcher);
}
