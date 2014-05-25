#ifndef __chck_atlas_h__
#define __chck_atlas_h__

typedef struct _chckAtlas chckAtlas;

chckAtlas* chckAtlasNew(void);
void chckAtlasFree(chckAtlas *atlas);
void chckAtlasReset(chckAtlas *atlas);
unsigned int chckAtlasCount(chckAtlas *atlas);
unsigned int chckAtlasPush(chckAtlas *atlas, const unsigned int width, const unsigned int height);
void chckAtlasPop(chckAtlas *atlas);
int chckAtlasPack(chckAtlas *atlas, const int forcePowerOfTwo, const int onePixelBorder, unsigned int *outWidth, unsigned int *outHeight);
int chckAtlasGetTextureLocation(const chckAtlas *atlas, const unsigned int index, unsigned int *outX, unsigned int *outY, unsigned int *outWidth, unsigned int *outHeight);

#endif /* __chck_atlas_h__ */

/* vim: set ts=8 sw=3 tw=0 :*/
