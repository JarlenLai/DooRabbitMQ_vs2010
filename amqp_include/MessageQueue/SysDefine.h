#ifndef	SYS_DEF_H
#define	SYS_DEF_H

	#if _MSC_VER >= 1000
		#pragma once
	#endif // _MSC_VER >= 1000

	#if defined(WIN32)
		#pragma warning(disable: 4251)
		#pragma warning(disable: 4275)
	#endif

	#if defined(WIN32)

		#define IMEXPORTS  

		#ifdef IMEXPORTS
			#define  Publish __declspec(dllexport)
		#else
			#define  Publish __declspec(dllimport)
		#endif

	#endif

    # if defined(__linux)
      #define  Publish 
    #endif

#endif			//all of end



#if defined (__linux)
	#include <sys/time.h>
#elif defined (WIN32)
	#include  <windows.h>
#endif