#ifndef GP_ASMLIB_H
#define GP_ASMLIB_H

void ASMZoomBlit(const unsigned char *src, unsigned char *dst4, int nbx, int nby, int height2, int zoom);
void ASMZoomTransBlit(const unsigned char *src, unsigned char *dst4, int nbx, int nby, int height2, int zoom, int trans);
void ASMZoomTransInvBlit(const unsigned char *src, unsigned char *dst4, int nbx, int nby, int height2, int zoom, int trans);
void ASMZoomSolidBlit(const unsigned char *src, unsigned char *dst4, int nbx, int nby, int height2, int zoom, int trans, int coul);
void ASMZoomSolidInvBlit(const unsigned char *src, unsigned char *dst4, int nbx, int nby, int height2, int zoom, int trans, int coul);
void ASMFastTransBlit(const unsigned char *src4, unsigned char *dst4, int nbx, int nby, int height2, int trans);
void ASMFastSolidBlit(const unsigned char *src4, unsigned char *dst4, int nbx, int nby, int height2, int trans, int coul);
void ASMSaveBitmap(const unsigned char *src4, unsigned char *dst, int nbx, int nby, int height2);
//void ASMMix(unsigned short *buf, s_mix *pmix, int len);
void ASMFastClear(unsigned char *dst4, int nbx, int nby);

#endif
