#ifndef BITFLAGS_H
#define BITFLAGS_H
#pragma once

#define INDEX_TO_FLAG(index) (1 << (index))

// Class that simplifies handling flags by using bit indices instead of masks.
// This allows one to employ a related enum made up by automatically assigned indices.
template<typename T = int>
class CBitFlags
{
	T mFlags;

public:
	CBitFlags(int flags = 0) : mFlags(flags) {}

	operator int()
	{
		return mFlags;
	}

	bool IsBitSet(int index) const
	{
		return !!(mFlags & INDEX_TO_FLAG(index));
	}

	template<typename... U>
	bool IsAnyBitSet(U... indices)
	{
		int indicesBuf[] = { indices... };

		for (auto index : indicesBuf)
		{
			if (IsBitSet(index))
			{
				return true;
			}
		}

		return false;
	}

	template<typename... U>
	bool AreAllBitsSet(U... indices)
	{
		int indicesBuf[] = { indices... };

		for (auto index : indicesBuf)
		{
			if (!IsBitSet(index))
			{
				return false;
			}
		}

		return true;
	}

	void SetBit(int index)
	{
		SetFlag(INDEX_TO_FLAG(index));
	}

	template<typename... U>
	void SetBits(U... indices)
	{
		int indicesBuf[] = { indices... };

		for (auto index : indicesBuf)
		{
			SetBit(index);
		}
	}

	void SetFlag(int flag)
	{
		mFlags |= flag;
	}

	void ClearBit(int index)
	{
		mFlags &= ~INDEX_TO_FLAG(index);
	}

	template<typename... U>
	void ClearBits(U... indices)
	{
		int indicesBuf[] = { indices... };

		for (auto index : indicesBuf)
		{
			ClearBit(index);
		}
	}
};

#endif // !BITFLAGS_H
