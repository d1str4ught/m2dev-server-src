#ifndef __INC_METIN_II_STL_H__
#define __INC_METIN_II_STL_H__

#include <vector>
#include <string>
#include <map>
#include <list>
#include <functional>
#include <stack>
#include <set>

#ifndef itertype
#define itertype(v) __typeof((v).begin())
#endif

inline void stl_lowers(std::string& rstRet)
{
	for (size_t i = 0; i < rstRet.length(); ++i)
		rstRet[i] = tolower(rstRet[i]);
}

template<typename T>
T* get_pointer(T* p) {
	return p;
}

namespace std
{
	template <class container, class Pred>
		void erase_if (container & a, typename container::iterator first, typename container::iterator past, Pred pred)
		{
			while (first != past)
				if (pred(*first))
					a.erase(first++);
				else
					++first;
		}

	template <class container>
		void wipe(container & a)
		{
			typename container::iterator first, past;

			first = a.begin();
			past = a.end();

			while (first != past)
				delete *(first++);

			a.clear();
		}

	template <class container>
		void wipe_second(container & a)
		{
			typename container::iterator first, past;

			first = a.begin();
			past = a.end();

			while (first != past)
			{
				delete first->second;
				++first;
			}

			a.clear();
		}
};

#endif
