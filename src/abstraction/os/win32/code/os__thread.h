#ifndef OS_WIN32__THREAD_H
#define OS_WIN32__THREAD_H

#if defined (__cplusplus)
extern "C" {
#endif

/** \brief Initialize thread module
 */
void
os_threadModuleInit (
    void);


/** \brief Deinitialize thread module
 */
void
os_threadModuleExit (
    void);


#if defined (__cplusplus)
}
#endif

#endif /* OS_WIN32__THREAD_H */