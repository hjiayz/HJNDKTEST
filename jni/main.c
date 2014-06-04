/*
 * Copyright (C) 2010 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

//BEGIN_INCLUDE(all)
#include <math.h>
#include <jni.h>
#include <errno.h>

#include <android/sensor.h>
#include <android/log.h>
#include <android_native_app_glue.h>
#include "native-audio-jni.h"
#include "font.h"
#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, "native-activity", __VA_ARGS__))
#define LOGW(...) ((void)__android_log_print(ANDROID_LOG_WARN, "native-activity", __VA_ARGS__))
#define LOGE(...) ((void)__android_log_print(ANDROID_LOG_ERROR,"native-activity", __VA_ARGS__))
#define STEPS 50
#define LHSIZE 100
#define LHCMAX 5 //礼花最大色彩数
#define LHMAX 3//同时释放的礼花数
#define PI 3.141592653589793238462643383279502884197169399
/**
 * Our saved state data.
 */
struct saved_state {
    float angle;
    int32_t x;
    int32_t y;
};
typedef struct lh_ {
    int x;
    int y;
    int z;
    int xmv;
    int ymv;
    int zmv;
    char light;
    char r;
    char g;
    char b;
} lh;
/**
 * Shared state for our app.
 */
struct engine {
    struct android_app* app;

    ASensorManager* sensorManager;
    const ASensor* accelerometerSensor;
    ASensorEventQueue* sensorEventQueue;

    int animating;
    int32_t width;
    int32_t height;
    struct saved_state state;
};
int fst;//first brush;
int thetime,fsz;
int ox,ox1;
int oy,oy1;
int oor,oog,oob;
int noor,noog,noob;
int ooor,ooog,ooob;
lh lhs[LHSIZE];
lh * lhsptr[LHSIZE];

int sxmv;
int symv;
int szmv;
int lhtime;

static void engine_init_display(struct engine* engine) {
	thetime=STEPS;
	lhtime=0;
	fst=1;

	noor=((rand())%255)/2;
	noog=((rand())%255)/2;
	noob=((rand())%255)/2;

	ooor=((rand())%255)/2;
	ooog=((rand())%255)/2;
	ooob=((rand())%255)/2;
	srand((unsigned)time(0));
	engine_draw_frame(engine);
}

void setcolor(uint8_t red,uint8_t green,uint8_t blue,int32_t format,void* px){
	if (format==WINDOW_FORMAT_RGBX_8888) {
		uint32_t A32Color=(blue<<16)+(green<<8)+(red);
		uint32_t* p=(uint32_t*)px;
		p[0]=A32Color;
		return;
	}
	if (format==WINDOW_FORMAT_RGB_565) {
		uint16_t A16Color=((red<<8)&63488)+((green<<3)&2016)+((blue>>3)&31);
		uint16_t* p=(uint16_t*)px;
		p[0]=A16Color;
		return;
	}
	if (format==WINDOW_FORMAT_RGBA_8888) {
		uint32_t A32Color=(blue<<16)+(green<<8)+(red);
		uint32_t* p=(uint32_t*)px;
		p[0]=A32Color;
		return;
	}
}
void brushpx(uint16_t x,uint16_t y,ANativeWindow_Buffer *buffer,uint8_t red,uint8_t green,uint8_t blue){
	if ((x<buffer->width)&(y<buffer->height)) {
		setcolor(red,green,blue,buffer->format,(y*buffer->stride+x)*(((buffer->format<=2)+1)<<1)+buffer->bits);
	}
}
uint16_t brushchar(uint16_t x,uint16_t y,int32_t Achar,uint8_t Asize,ANativeWindow_Buffer *buffer,uint8_t red,uint8_t green,uint8_t blue) {
	int errinfo=setfontsize(Asize,Asize);
    if (errinfo>0) {
    	LOGE("setfontsize error %d",errinfo);
    }
    errinfo=setftchar(Achar);
    if (errinfo>0){
    	LOGE("getftchar %d",errinfo);
    }
    FT_Bitmap AFTBitmap=face->glyph->bitmap;
    int w=AFTBitmap.width;
    int h=AFTBitmap.rows;
    int i,j,k=0;
    for(i=0;i<h;i++) {
    	for(j=0;j<w;j++){
    		if (AFTBitmap.buffer[k]>0) {
    			//水平移动brushpx(((j+x+ox*(STEPS-thetime)/STEPS)/2),((i+y+(Asize-h+oy*(STEPS-thetime)/STEPS))/2),buffer,AFTBitmap.buffer[k]*red/255,AFTBitmap.buffer[k]*green/255,AFTBitmap.buffer[k]*blue/255);
    			brushpx(((int)((j+x)*((double)thetime/STEPS))+(int)(ox*((double)(STEPS-thetime)/STEPS))),((int)((i+y+(Asize-h))*(double)thetime/STEPS)+(int)(oy*(double)(STEPS-thetime)/STEPS)),buffer,(AFTBitmap.buffer[k]*red/255+(255-AFTBitmap.buffer[k])*oor/255)/1,(AFTBitmap.buffer[k]*green/255+(255-AFTBitmap.buffer[k])*oog/255)/1,(AFTBitmap.buffer[k]*blue/255+(255-AFTBitmap.buffer[k])*oob/255)/1);
    		}
    		k++;
    	}
    }
    return w;
//返回宽度
}
int brushstring(uint16_t x,uint16_t y,int32_t Astring[],uint8_t Asize,uint8_t spacing,ANativeWindow_Buffer *buffer,uint8_t red,uint8_t green,uint8_t blue) {
	int i,l;
	for(i=1,l=x;i<=(Astring[0]);i++){
		l+=brushchar(l,y,Astring[i],Asize,buffer,red,green,blue)+spacing;
	}
	return (l-x);
}
/**
 * Just the current frame in the display.
 */
void brushbackground(ANativeWindow_Buffer *buffer,uint8_t red,uint8_t green,uint8_t blue) {
	uint32_t i,j;
	for(i=0;i<buffer->height;i++) {
		for(j=0;j<buffer->width;j++) {
			brushpx(j,i,buffer,red,green,blue);
		}
	}
}
//礼花
int lhcomp(const void *a,const void *b)
{
	return ((lh*)a)->zmv-((lh*)b)->zmv;
}
uint16_t getcircleY(int x,int r){
	return (int)sqrt((double)((r*r)-(x*x)));
}
void brushlineY(int x,int y1,int y2,ANativeWindow_Buffer *buffer,int red,int green,int blue){//沿着Y方向从上向下画线
	int i;
	for(i=y1;i<=y2;i++){
		brushpx(x,i,buffer,red,green,blue);
	}
}
void brushcircle(uint16_t x,uint16_t y,uint16_t r,ANativeWindow_Buffer *buffer,uint8_t red,uint8_t green,uint8_t blue){
	uint16_t cx=0,cy;
	while (cx<=r)
	{
		cy=getcircleY(cx,r);
		brushlineY(x+cx,y-cy,y+cy,buffer,red,green,blue);
		brushlineY(x-cx,y-cy,y+cy,buffer,red,green,blue);
		//brushpx(x+cx,y+cy,buffer,red,green,blue);
		//brushpx(x-cx,y-cy,buffer,red,green,blue);
		//brushpx(x+cx,y-cy,buffer,red,green,blue);
		//brushpx(x-cx,y+cy,buffer,red,green,blue);
		cx++;
	}
}
void brushlhpoint(int item,ANativeWindow_Buffer *buffer){

	//LOGE("setlight %d %d",item,(lhsptr[item])->light);
	int looklight=(lhsptr[item]->light)-(lhsptr[item]->z/100);//视亮度
	if (looklight<0) {looklight=0;}
	if (looklight>255) {looklight=255;}
	int imax=looklight/20;
	int i,j;
	int realred,realgreen,realblue;
	for(j=imax,i=0;i<imax;i++,j--){
		//LOGE("%d",imax);
		realred=((lhsptr[item]->r)*i*20+oor*(255-i*20))/255;
		realgreen=((lhsptr[item]->g)*i*20+oog*(255-i*20))/255;
		realblue=((lhsptr[item]->b)*i*20+oob*(255-i*20))/255;
		//LOGE("brushc %d %d %d",lhsptr[item]->x,lhsptr[item]->y,lhsptr[item]->z);
		brushcircle(lhsptr[item]->x,lhsptr[item]->y,j,buffer,realred,realgreen,realblue);
		//LOGE("brushcd %d %d %d",lhsptr[item]->xmv,lhsptr[item]->ymv,lhsptr[item]->zmv);
	}
}
void mvlhpoint(int item){
	lhsptr[item]->light-=255/STEPS*3;
	if ((lhsptr[item]->light)<(0)) {lhsptr[item]->light=0;}
	lhsptr[item]->x+=lhsptr[item]->xmv;
	lhsptr[item]->y+=lhsptr[item]->ymv;
	lhsptr[item]->z+=lhsptr[item]->zmv;
	lhsptr[item]->ymv+=15;
}
void brushlihua(ANativeWindow_Buffer *buffer){
	int i;
	for(i=0;i<LHSIZE;i++){
		//LOGE("brushp %d",i);
		brushlhpoint(i,buffer);
		//LOGE("brushpd");
		mvlhpoint(i);
		//LOGE("mvend");
	}
}

void initlihua(int w,int h){
	double c1,c2;//角度 c1 xypingmian c2 x z pingmian
	unsigned f;//方向
	int i,j;
	int colorname;
	int colorchoose;
	int x,y,z,r;
	uint32_t colorlist[LHCMAX];
	unsigned temr,temg,temb;
	int lhnum=(rand())%(LHMAX)+1;
	//LOGE("%d",lhnum);
	for(i=0;i<LHSIZE;i++){
		//LOGE("%d %d %d",LHSIZE,lhnum,i);
		if ((i%(LHSIZE/lhnum+1))==0) {
			x=rand()%(w-100)+50;
			y=rand()%(h/2);
			z=50+rand()%(int)50;
			r=rand()%20+20;
			sxmv=-(rand()%(int)20)+10;
			symv=-(rand()%(int)20)-10;
			szmv=-(rand()%(int)20)+10;

			colorchoose=rand()%LHCMAX;//色彩数
			for(j=0;j<LHCMAX;j++){
				temr=(rand())%((int)4)*64+63;
				temg=(rand())%((int)4)*64+63;
				if ((temr+temg)<255) {
					temb=((rand())%((int)3)+1)*64+63;
				}
				else {
					temb=((rand())%((int)3))*64+63;
				}

				LOGE("%d %d %d",temr,temg,temb);

				colorlist[j]=(temr<<16)+(temg<<8)+temb;
			}
		}
		lhs[i].x=x;
		lhs[i].y=y;
		lhs[i].z=z;
		c1=(rand()%360)*PI/4;//仰角
		c2=(rand()%360)*PI/4;//xz平面基于x轴夹角
		f=i*8/LHSIZE;

		//x*x+y*y+z*z=r*r;
		lhs[i].xmv=(int)(cos(c1)*cos(c2)*r*((f>>2)?1:-1))+sxmv;
		lhs[i].zmv=(int)(sin(c2)*cos(c1)*r*(((f>>1)&1)?1:-1))+szmv;
		lhs[i].ymv=(int)(sqrt(r*r-lhs[i].xmv*lhs[i].xmv-lhs[i].zmv*lhs[i].zmv)*((f&1) ? 1 : -1 ))+symv;
		lhs[i].light=rand()%10+246;
		colorname=rand()%(colorchoose+1);
		lhs[i].r=(colorlist[colorname]&0xFF0000)>>16;
		lhs[i].g=(colorlist[colorname]&0x00FF00)>>8;
		lhs[i].b=(colorlist[colorname]&0x0000FF);
		lhsptr[i]=&lhs[i];

	}

    //LOGE("qsortf");
	qsort(lhsptr,LHSIZE,sizeof(lhsptr[0]),lhcomp);
    //LOGE("qsortd");
}
//礼花
engine_draw_frame(struct engine* engine) {
    if (engine->app->window == NULL) {
        // No window.
        return;
    }
    ANativeWindow_Buffer buffer;
    if (ANativeWindow_lock(engine->app->window, &buffer, NULL) < 0) {
     //   LOGW("Unable to lock window buffer");
        return;
    }
    engine->width=buffer.width;
    engine->height=buffer.height;
    /* Now fill the values with a nice little plasma */
    thetime++;
    lhtime++;
    if (lhtime>(STEPS/3)){lhtime=0;fst=1;}
    if (fst) {initlihua(buffer.width,buffer.height);}//生成礼花
    fst=0;
    if (thetime>STEPS) {
    	thetime=0;
    	ooor=noor;
    	ooog=noog;
    	ooob=noob;
    	noor=((rand())%255)/2;
    	noog=((rand())%255)/2;
    	noob=((rand())%255)/2;
    	ox=rand()%buffer.width;
    	oy=rand()%buffer.height;
    	ox1=rand()%(buffer.width-200);
    	oy1=rand()%(buffer.height-10);
    	fsz=30+(rand())%70;
    }
    oor=(noor*thetime/STEPS)+(ooor*(STEPS-thetime)/STEPS);
    oog=(noog*thetime/STEPS)+(ooog*(STEPS-thetime)/STEPS);
    oob=(noob*thetime/STEPS)+(ooob*(STEPS-thetime)/STEPS);
    //thetime=STEPS;
    //LOGE("%d",buffer.format);
    brushbackground(&buffer,oor,oog,oob);
    //brushchar(30,70,0x4E2D,50,&buffer,255,0,0);
    //uint32_t word[]={13,0x4E16,0x754C,0x4EBA,0x6C11,0x5927,0x56E2,0x7ED3,0x4E07,0x5C81,'G','o','o','d'};
    uint32_t word[]={4,26032,23130,24555,20048};
    brushstring(ox1,oy1,word,fsz,2,&buffer,255-oor,255-oog,255-oob);
    //LOGE("brushlihua");
    brushlihua(&buffer);
    //LOGE("brushlihuad");
    ANativeWindow_unlockAndPost(engine->app->window);
}

/**
 * Tear down the EGL context currently associated with the display.
 */
static void engine_term_display(struct engine* engine) {
    engine->animating = 0;
}




/**
 * Process the next input event.
 */
static int32_t engine_handle_input(struct android_app* app, AInputEvent* event) {
    struct engine* engine = (struct engine*)app->userData;
    if (AInputEvent_getType(event) == AINPUT_EVENT_TYPE_MOTION) {
        engine->animating = 1;
        return 1;
    } else if (AInputEvent_getType(event) == AINPUT_EVENT_TYPE_KEY) {
        LOGI("Key event: action=%d keyCode=%d metaState=0x%x",
                AKeyEvent_getAction(event),
                AKeyEvent_getKeyCode(event),
                AKeyEvent_getMetaState(event));
    }

    return 0;
}

/**
 * Process the next main command.
 */
static void engine_handle_cmd(struct android_app* app, int32_t cmd) {
    struct engine* engine = (struct engine*)app->userData;
    switch (cmd) {
        case APP_CMD_SAVE_STATE:
            // The system has asked us to save our current state.  Do so.
            //engine->app->savedState = malloc(sizeof(struct saved_state));
            //*((struct saved_state*)engine->app->savedState) = engine->state;
            //engine->app->savedStateSize = sizeof(struct saved_state);
            //engine_draw_frame(engine);
            break;
        case APP_CMD_INIT_WINDOW:
            // The window is being shown, get it ready.
            if (engine->app->window != NULL) {
                engine_init_display(engine);
                setPlayingAssetAudioPlayer(1);
            }
            break;
        case APP_CMD_TERM_WINDOW:
            // The window is being hidden or closed, clean it up.
            engine_term_display(engine);
            break;
        case APP_CMD_GAINED_FOCUS:
            // When our app gains focus, we start monitoring the accelerometer.
            if (engine->accelerometerSensor != NULL) {
                ASensorEventQueue_enableSensor(engine->sensorEventQueue,
                        engine->accelerometerSensor);
                // We'd like to get 60 events per second (in us).
                ASensorEventQueue_setEventRate(engine->sensorEventQueue,
                        engine->accelerometerSensor, (1000L/60)*1000);
            }
            engine->animating = 1;
            setPlayingAssetAudioPlayer(1);
            break;
        case APP_CMD_LOST_FOCUS:
            // When our app loses focus, we stop monitoring the accelerometer.
            // This is to avoid consuming battery while not being used.
            if (engine->accelerometerSensor != NULL) {
                ASensorEventQueue_disableSensor(engine->sensorEventQueue,
                        engine->accelerometerSensor);
            }
            // Also stop animating.
            engine->animating = 0;
            //setPlayingAssetAudioPlayer(0);
            //engine_draw_frame(engine);
            break;
        case APP_CMD_DESTROY:

            shutdown();
            freefont();
        	break;
    }
}

/**
 * This is the main entry point of a native application that is using
 * android_native_app_glue.  It runs in its own thread, with its own
 * event loop for receiving input events and doing other things.
 */
void android_main(struct android_app* state) {
    struct engine engine;
    int x=0,y=0,z=0;
    // Make sure glue isn't stripped.
    app_dummy();

    memset(&engine, 0, sizeof(engine));
    state->userData = &engine;
    state->onAppCmd = engine_handle_cmd;
    state->onInputEvent = engine_handle_input;
    engine.app = state;

    // Prepare to monitor accelerometer
    engine.sensorManager = ASensorManager_getInstance();
    engine.accelerometerSensor = ASensorManager_getDefaultSensor(engine.sensorManager,
            ASENSOR_TYPE_ACCELEROMETER);
    engine.sensorEventQueue = ASensorManager_createEventQueue(engine.sensorManager,
            state->looper, LOOPER_ID_USER, NULL, NULL);


    int errinfo=initfont();
    if (errinfo>0) {

        LOGE("fontinit error %d",errinfo);
    }
    //LOGE("initfont %d",errinfo);

    //if (state->savedState != NULL) {
        // We are starting with a previous saved state; restore from it.
      //  engine.state = *(struct saved_state*)state->savedState;
    //}

    // loop waiting for stuff to do.

    createEngine();
    createAssetAudioPlayer(engine.app->activity->assetManager,"2.mp3");
    while (1) {
        // Read all pending events.
        int ident;
        int events;
        struct android_poll_source* source;

        // If not animating, we will block forever waiting for events.
        // If animating, we loop until all events are read, then continue
        // to draw the next frame of animation.
        while ((ident=ALooper_pollAll(engine.animating ? 0 : -1, NULL, &events,
                (void**)&source)) >= 0) {
            // Process this event.
            if (source != NULL) {
                source->process(state, source);
            }

            // If a sensor has data, process it now.
            if (ident == LOOPER_ID_USER) {
                if (engine.accelerometerSensor != NULL) {
                    ASensorEvent event;
                    while (ASensorEventQueue_getEvents(engine.sensorEventQueue,
                            &event, 1) > 0) {
                 //       LOGI("accelerometer: x=%f y=%f z=%f",
                 //               event.acceleration.x, event.acceleration.y,
                 //               event.acceleration.z);
                    }
                }
            }

            // Check if we are exiting.
            if (state->destroyRequested != 0) {
                engine_term_display(&engine);
                return;
            }
        }

        if (engine.animating) {
            // Done with events; draw next animation frame.
            //engine.state.angle += .01f;
            //if (engine.state.angle > 1) {
              //  engine.state.angle = 0;
            //}

            // Drawing is throttled to the screen update rate, so there
            // is no need to do timing here.
            engine_draw_frame(&engine);
        }
    }
}
//END_INCLUDE(all)
