#pragma once


#define CFG_TYPES \
CFG_T(char*, edv_string) \
CFG_T(int, edv_int) \
CFG_T(struct { \
	char r; \
	char g; \
	char b; \
	char a; \
}, edv_color) \


#define CFG_T(x,y) typedef x y;
CFG_TYPES
#undef CFG_TYPE


//these are the edv_type with _cfg added
typedef enum {

#define CFG_T(x,y) y##_cfg,
	CFG_TYPES
#undef CFG_TYPE

}cfg_type;

//first arg - type
//second arg - name
//third arg - default type (technically a list of var args but looks exactly the same as an initialiser for the type specified)
#define  CFG_LIST \
CFG(edv_color,text_color,{255,255,255,255} ) \
CFG(edv_color,menu_color,{255,255,255,255} ) \
CFG(edv_color,background_color,{0,0,0,255} ) \
CFG(edv_color,line_number_color,{150,150,150,255} ) \
CFG(edv_string,default_font,"assets/CascadiaMono-Regular.otf" ) \
CFG(edv_int,font_size,18 ) \



typedef struct {
#define CFG(x,arg,y) x arg ;
	CFG_LIST
#undef CFG
} edv_config;

void unload_config(edv_config* cfg);
edv_config* load_config(void);