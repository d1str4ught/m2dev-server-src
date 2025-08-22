#ifndef __INC_SINGLETON_H__
#define __INC_SINGLETON_H__

#include <assert.h>

template <typename T> class singleton 
{ 
	public: 
		static T * ms_singleton;

		singleton()
		{ 
			assert(!ms_singleton);
			intptr_t offset = (intptr_t) (T*) 1 - (intptr_t) (singleton <T>*) (T*) 1;
			ms_singleton = (T*) ((intptr_t) this + offset);
		} 

		virtual ~singleton()
		{ 
			assert(ms_singleton);
			ms_singleton = 0; 
		}

		static T & instance()
		{
			assert(ms_singleton);
			return (*ms_singleton);
		}

		static T & Instance()
		{
			assert(ms_singleton);
			return (*ms_singleton);
		}

		static T * instance_ptr()
		{
			return (ms_singleton);
		}
};

template <typename T> T * singleton <T>::ms_singleton = NULL;

#endif
