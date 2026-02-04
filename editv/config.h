#pragma once

typedef char* edv_string;
typedef int edv_int;

#define  CFG_LIST \
CFG(edv_color,text_color,) \
CFG(edv_color,menu_color,) \
CFG(edv_color,background_color,) \
CFG(edv_color,line_number_color,) \
CFG(edv_string,default_font,) \
CFG(edv_int,font_size,) \

typedef struct {
	char r;
	char g;
	char b;
	char a;
} edv_color;

typedef struct {
#define CFG(x,arg,y) x arg y ;
	CFG_LIST
#undef CFG
} edv_config;

void unload_config(edv_config* cfg);
edv_config* load_config(void);