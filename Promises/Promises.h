#pragma once

#include <stdio.h>
#include <optional>
#include <SupportDefs.h>
#include <Referenceable.h>
#include <Locker.h>
#include <Autolock.h>


template <typename ResolvedType, typename RejectedType>
class Promise: public BReferenceable {
private:	
	struct PromiseItem {
		PromiseItem *next;
		virtual void Do(Promise &promise) = 0;
		PromiseItem(): next(NULL) {}
		virtual ~PromiseItem() {}
	};
	
	template <typename FnType>
	struct PromiseItemImpl final: public PromiseItem {
		FnType fDoFn;
		void Do(Promise &promise) final {fDoFn(promise);}
		PromiseItemImpl(FnType doFn): fDoFn(doFn) {}
	};

	BLocker fLocker;
	std::optional<ResolvedType> fResolvedVal;
	std::optional<RejectedType> fRejectedVal;
	PromiseItem *fOnResolve;
	PromiseItem *fOnReject;

public:
	Promise(): fOnResolve(NULL), fOnReject(NULL)
	{BAutolock lock(fLocker);}

	~Promise()
	{
		printf("-Promise()\n");
		BAutolock lock(fLocker);
		while (fOnResolve != NULL) {
			PromiseItem *next = fOnResolve->next;
			delete fOnResolve;
			fOnResolve = next;
		}
		while (fOnReject != NULL) {
			PromiseItem *next = fOnReject->next;
			delete fOnReject;
			fOnReject = next;
		}
	}

	bool IsResolved() {BAutolock lock(fLocker); return fResolvedVal.has_value();}
	bool IsRejected() {BAutolock lock(fLocker); return fRejectedVal.has_value();}
	bool IsSet() {BAutolock lock(fLocker); return IsResolved() || IsRejected();}
	ResolvedType &ResolvedValue() {BAutolock lock(fLocker); return *fResolvedVal;}
	RejectedType &RejectedValue() {BAutolock lock(fLocker); return *fRejectedVal;}

	template< class... Args >
	bool Resolve(Args&&... args)
	{
		BAutolock lock(fLocker);
		if (IsSet()) return false;
		fResolvedVal.emplace(std::forward<Args>(args)...);
		for (PromiseItem *item = fOnResolve; item != NULL; item = item->next) {
			item->Do(*this);
		}
		return true;
	}

	template< class... Args >
	bool Reject(Args&&... args)
	{
		BAutolock lock(fLocker);
		if (IsSet()) return false;
		fRejectedVal.emplace(std::forward<Args>(args)...);
		for (PromiseItem *item = fOnReject; item != NULL; item = item->next) {
			item->Do(*this);
		}
		return true;
	}

	template <typename DoFn>
	void OnResolve(DoFn callback)
	{
		BAutolock lock(fLocker);
		if (fResolvedVal.has_value()) {
			return callback(*this);
		}
		if (!fRejectedVal.has_value()) {
			PromiseItem *item = new PromiseItemImpl<DoFn>(callback);
			item->next = fOnResolve; fOnResolve = item;
		}
	}

	template <typename DoFn>
	void OnReject(DoFn callback)
	{
		BAutolock lock(fLocker);
		if (fRejectedVal.has_value()) {
			return callback(*this);
		}
		if (!fResolvedVal.has_value()) {
			PromiseItem *item = new PromiseItemImpl<DoFn>(callback);
			item->next = fOnReject; fOnReject = item;
		}
	}

	template <typename DoFn>
	void OnSet(DoFn callback)
	{
		BAutolock lock(fLocker);
		OnResolve(callback);
		OnReject(callback);
	}

};
