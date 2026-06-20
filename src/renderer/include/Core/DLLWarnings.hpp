#ifndef CORE_DLL_WARNINGS_HPP
#define CORE_DLL_WARNINGS_HPP

#ifdef _MSC_VER

#define SUPPRESS_DLL_INTERFACE_WARNINGS_BEGIN \
    __pragma(warning(push)) \
    __pragma(warning(disable: 4251)) /* class 'std::xxx' needs to have dll-interface */ \
    __pragma(warning(disable: 4275)) /* non dll-interface class 'std::xxx' used as base */

#define SUPPRESS_DLL_INTERFACE_WARNINGS_END \
    __pragma(warning(pop))

#define SUPPRESS_STL_DLL_WARNINGS_BEGIN SUPPRESS_DLL_INTERFACE_WARNINGS_BEGIN
#define SUPPRESS_STL_DLL_WARNINGS_END SUPPRESS_DLL_INTERFACE_WARNINGS_END

#else
#define SUPPRESS_DLL_INTERFACE_WARNINGS_BEGIN
#define SUPPRESS_DLL_INTERFACE_WARNINGS_END
#define SUPPRESS_STL_DLL_WARNINGS_BEGIN
#define SUPPRESS_STL_DLL_WARNINGS_END
#endif

#endif // CORE_DLL_WARNINGS_HPP
