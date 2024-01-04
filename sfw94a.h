#pragma once

int fix_mirroring(char* filename);
char* get_date_from_sfw(USCH* sfwstart, USCH* sfwend);
int add_develop_date_to_jpg(char* filename, char* exifdate);
