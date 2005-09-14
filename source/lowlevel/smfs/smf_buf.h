#ifndef SMF_BUF_H_
#define SMF_BUF_H_

#if (SM_OS == MS_WINNT || SM_OS == MS_WINDOWS) && defined(_DEBUG)
#define SMB_ALLOC_BUF()		smbAllocBuf(__FILE__, __LINE__)
#define SMB_FREE_BUF(buf)	smbFreeBuf((buf), __FILE__, __LINE__)
#else
#define SMB_ALLOC_BUF()		smbAllocBuf()
#define SMB_FREE_BUF(buf)	smbFreeBuf((buf))
#endif

extern void smbBufPoolInit(void);

#if (SM_OS == MS_WINNT || SM_OS == MS_WINDOWS) && defined(_DEBUG)
extern ubyte* smbAllocBuf(const ubyte* file, int line);
#else
extern ubyte* smbAllocBuf(void);
#endif

#if (SM_OS == MS_WINNT || SM_OS == MS_WINDOWS) && defined(_DEBUG)
extern void smbFreeBuf(ubyte* buf, const ubyte* file, int line);
#else
extern void smbFreeBuf(ubyte* buf);
#endif

#endif	/* SMF_BUF_H_ */
