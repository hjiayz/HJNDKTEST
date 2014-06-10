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
#include <stdlib.h>
#include <jni.h>
#include <errno.h>
#include <pthread.h>
#include <android/sensor.h>
#include <android/log.h>
#include <android_native_app_glue.h>
//#include "native-audio-jni.h"
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
/*typedef struct lh_ {
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
*/
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
//int noor,noog,noob;
//int ooor,ooog,ooob;
//lh lhs[LHSIZE];
//lh * lhsptr[LHSIZE];
int pausemakevalue;
int kind,stopkind;
int Truning;
int brushvalue,thisvalue;
pthread_t pt;
//int brushbg;
//int sxmv;
//int symv;
//int szmv;
//int lhtime;
char tv[16][3]={
		{1,3},
		{3,1},
		{4,6},
		{6,4},
		{7,9},
		{9,7},
		{1,7},
		{7,1},
		{2,8},
		{8,2},
		{3,9},
		{9,3},
		{1,9},
		{9,1},
		{3,7},
		{7,3}
};
void inittv(){
    int i;
    for (i=0;i<16;i++){
            tv[i][2]=(tv[i][0]+tv[i][1])/2;
    }

}
int testvalue(int value){
        char tem[9];
        int i,j,k,chker;
        for(i=0;i<10;i++){
                tem[i]=value%10;
                value/=10;
        }
        //检测是否在0之后有其他数字,要求0必须在所有数字之后
        chker=0;
        for(i=0;i<10;i++){
                if (tem[i]==0) {
                        chker=1;
                }
                else {
                        if (chker==1){

                                return i-1;
                        }
                }
        }
        //检测相同数字
        for(i=0;i<10;i++){
                for(j=0;j<i;j++){
                        if ((tem[i]!=0)&&(tem[j]==tem[i])) {return j;}
                }
        }
        //检测不存在路径
        for(i=0;i<9;i++){
                for(j=0;j<16;j++){
                        chker=1;
                        if ((tem[i]==tv[j][0])&&(tem[i+1]==tv[j][1])) {
                                for (k=i+1;k<10;k++) {
                                        if (tem[k]==tv[j][2]){
                                                chker=0;
                                                break;
                                        }
                                }
                                if (chker) {return i;}
                        }
                }
        }
        return -10;
}


static void engine_init_display(struct engine* engine) {

	thetime=STEPS;
	//lhtime=0;
	fst=1;
	//brushbg=1;
	//noor=((rand())%255)/2;
	//noog=((rand())%255)/2;
	//noob=((rand())%255)/2;

	//ooor=((rand())%255)/2;
	//ooog=((rand())%255)/2;
	//ooob=((rand())%255)/2;
	oor=0;
	oog=0;
	oob=0;
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
    int wt=w;
    if (wt==0) {wt=1;}
    int i,j,k=0;
    for(i=0;i<h;i++) {
    	for(j=0;j<w;j++){
    		if (AFTBitmap.buffer[k]>0) {
    			//j 文字内偏移 x基准位置 ox新位置

    			//LOGE("me");
    			brushpx( x+j ,y+((Asize-h)/2)+i ,buffer,red,green,blue);
    			//brushpx( x+j ,y+((Asize-h)/2)+i ,buffer,red,green*j/wt,blue);
    			//brushpx(((int)((j+x)*((double)thetime/STEPS))+(int)((ox+x)*((double)(STEPS-thetime)/STEPS))),((int)((i+y+(Asize-h))/**(double)thetime/STEPS)*/+(int)((oy+y)*(double)(STEPS-thetime)/STEPS)),buffer,(AFTBitmap.buffer[k]*red/255+(255-AFTBitmap.buffer[k])*oor/255)/1,(AFTBitmap.buffer[k]*green/255+(255-AFTBitmap.buffer[k])*oog/255)/1,(AFTBitmap.buffer[k]*blue/255+(255-AFTBitmap.buffer[k])*oob/255)/1);
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
		//l+=+spacing;
		l+=((brushchar(l,y,Astring[i],Asize,buffer,red,green,blue)<(Asize/2+10))?(Asize/2):Asize);
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
/*int lhcomp(const void *a,const void *b)
{
	return ((lh*)a)->zmv-((lh*)b)->zmv;
}
*/
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
/*void brushlhpoint(int item,ANativeWindow_Buffer *buffer){

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
 *
 */
//检测
int cut(int a,int b){
        int p=(int)pow(10,b);
       // LOGE("%d\n",p);
        return a-a%p+(int)pow(10,b);
}
/*
 int getkind(){
         int i,count=0,t;
         for (i=1;i<1000000000;){
 //              printf("%d\n",i);
                 t=testvalue(i);
                 if (t==-10) {
                         count++;
                         //printf("%d\n",i);
                         i++;
                 }
                 else {
                         i=cut(i,t);
                 //      i++;
                 }
                 //LOGE("%d\n",i);
         }

         return count;
}
*/
void* makevalue(){
	int t;
	while (Truning) {
		if (brushvalue<1000000000){
			t=testvalue(brushvalue);
			if (t==-10) {
				if (stopkind==0) {
					kind++;
				}
				else{
					usleep(100000);
				}
				while (pausemakevalue) {
					usleep(10000);
				}
				thisvalue=brushvalue;

				brushvalue++;
			}
			else {
				brushvalue=cut(brushvalue,t);
			}
		}
		else {
			brushvalue=1;
			stopkind=1;
		}
	}
	pthread_exit(NULL);
	return NULL;
}
void brushline(ANativeWindow_Buffer *buffer,int x1,int y1,int x2,int y2,uint8_t red,uint8_t green,uint8_t blue,int width){
	int i,j,m,f;//m move value f 0横向 1 纵向
	f=1;
	if (abs(x1-x2)>abs(y1-y2)) {
		f=0;
	}
	m=-1;

	//LOGE("m1");
	if (!f){
		if (x1<x2) {m=1;}
		for(i=x1;i!=x2;i+=m){

			//LOGE("m2");
			for(j=(-width/2);j<=(width/2);j++){
				brushpx(i,(((i-x1)*(y2-y1)/(x2-x1))+y1)+j,buffer,red,green,blue);
			}
		}
		brushpx(x2,y2,buffer,red,green,blue);

		//LOGE("m3");
	}
	else {
		if (y1<y2) {m=1;}
		for(i=y1;i!=y2;i+=m){

//			LOGE("m4");
			for(j=(-width/2);j<=(width/2);j++){
				brushpx((i-y1)*(x2-x1)/(y2-y1)+x1+j,i,buffer,red,green,blue);
			}
	//		LOGE("m5");
		}

		//LOGE("m6");
		brushpx(x2,y2,buffer,red,green,blue);

		//LOGE("m7");
	}
}
void brushliner(ANativeWindow_Buffer *buffer,int x1,int y1,int x2,int y2,uint8_t red,uint8_t green,uint8_t blue,int width,int r){
	int rx1,rx2,ry1,ry2,tem;
	int len=sqrt((x1-x2)*(x1-x2)+(y1-y2)*(y1-y2));
	if (y1<y2) {
		tem=r/len*(y2-y1);
		ry1=y1+tem;
		ry2=y2-tem;
	}
	else {
		tem=r/len*(y1-y2);
		ry1=y1-tem;
		ry2=y2+tem;
	}
	if (x1<x2) {
		tem=r/len*(x2-x1);
		rx1=x1+tem;
		rx2=x2-tem;
	}
	else {
		tem=r/len*(x1-x2);
		rx1=x1-tem;
		rx2=x2+tem;
	}
	brushline(buffer,rx1,ry1,rx2,ry2,red,green,blue,width);
}
void brushthisvalue(ANativeWindow_Buffer *buffer){
	//zuobiao list
	int width=buffer->width;
	int height=buffer->height;
	int r;
	if (height>width) {
		r=width/8;
	}
	else {
		r=height/8;
	}
	int fsize=r*3/2,ftop=fsize/2,fleft=fsize/4;
	int zb[9][2],i;
	for(i=0;i<9;i++){
		zb[i][0]=width*(((i*2)%6)+1)/6;
		zb[i][1]=height*(i/3*2+1)/6;
		brushcircle(zb[i][0],zb[i][1],r,buffer,0xFF,0xFF,0xFF);
		brushchar(zb[i][0]-fleft,zb[i][1]-ftop,'1'+i,fsize,buffer,0x88,0x88,0x88);
	}
	//LOGE("mc");
	int tem=thisvalue,tembit,temx,temy,j,r2=r/6;
	for(j=100000000;(tem/j)==0;j/=10);
	for(i=0;;i++){
		if (j==0) {
			brushcircle(temx,temy,r2,buffer,0x0,0x0,0xFF);
			break;
		}
		tembit=tem/j;
		if (i>1) {
			brushcircle(temx,temy,r2,buffer,0x0,0xFF,0x0);
		//	LOGE("ma2");
		}
		//LOGE("mt");
		if (i!=0) {

			//LOGE("mb");
			brushliner(buffer,temx,temy,zb[tembit-1][0],zb[tembit-1][1],0,255,0,5,r2+5);
			//brushcircle(zb[tembit-1][0],zb[tembit-1][1],r/3,buffer,0x0,0xFF,0x0);
			//brushchar(zb[tembit-1][0]-4,zb[tembit-1][1]-9,'1'+i,18,buffer,255,255,255);
		}
		if (i==1) {
			brushcircle(temx,temy,r2,buffer,0xFF,0x0,0x0);
		}
		//if (i==1){
		//}
		temx=zb[tembit-1][0];
		temy=zb[tembit-1][1];
		tem=tem%j;
		j/=10;
	}
	tem=thisvalue;
	fsize=r/4;
	ftop=fsize/2;
	fleft=fsize/4;
	for(j=100000000;(tem/j)==0;j/=10);
	for(i=0;;i++){
		if (j==0) {
			brushchar(temx-fleft,temy-ftop,'0'+i,fsize,buffer,0xFF,0x0,0xFF);
			return;
		}
		tembit=tem/j;
		//LOGI("%d",tem);
		//LOGI("%d",j);
		if (i==1) {
			brushchar(temx-fleft,temy-ftop,'0'+i,fsize,buffer,0x0,0x0,0x0);
		}
		if (i>1) {
			brushchar(temx-fleft,temy-ftop,'0'+i,fsize,buffer,0xFF,0x0,0x0);
		}
		temx=zb[tembit-1][0];
		temy=zb[tembit-1][1];
		tem=tem%j;
		j/=10;
	}

}
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
//    lhtime++;
//    if (lhtime>(STEPS/3)){lhtime=0;fst=1;}
    if (fst) {
    	ox=rand()%(buffer.width-500);
    	oy=rand()%(buffer.height-100);
    }
    fst=0;
    if (thetime>=STEPS) {
    	thetime=0;
    	/*ooor=noor;
    	ooog=noog;
    	ooob=noob;
    	noor=((rand())%255)/2;
    	noog=((rand())%255)/2;
    	noob=((rand())%255)/2;
    	*/
    	ox1=ox;
    	oy1=oy;
    	ox=rand()%(buffer.width-500);
    	oy=rand()%(buffer.height-100);
    	//fsz=30+(rand())%70;
    }
    /*
    oor=(noor*thetime/STEPS)+(ooor*(STEPS-thetime)/STEPS);
    oog=(noog*thetime/STEPS)+(ooog*(STEPS-thetime)/STEPS);
    oob=(noob*thetime/STEPS)+(ooob*(STEPS-thetime)/STEPS);
    */
    //thetime=STEPS;
    //LOGE("%d",buffer.format);
    brushbackground(&buffer,oor,oog,oob);
    brushthisvalue(&buffer);
    //brushchar(30,70,0x4E2D,50,&buffer,255,0,0);
    //uint32_t word[]={13,0x4E16,0x754C,0x4EBA,0x6C11,0x5927,0x56E2,0x7ED3,0x4E07,0x5C81,'G','o','o','d'};
    //uint32_t word[]={4,26032,23130,24555,20048};
    uint32_t *wordptr;
    wordptr=(uint32_t *)malloc(sizeof(uint32_t)*20);
    int i;
    wordptr[1]=0x4E00;
    wordptr[2]=0x5171;
    wordptr[3]=0x6709;
    int kindtem=kind;

    for(i=4;kindtem!=0;i++){
    	wordptr[i]=kindtem%10+'0';
    	kindtem/=10;
    }
    wordptr[i]=0x79CD;
    wordptr[i+1]=0x7EC4;
    wordptr[i+2]=0x5408;
    wordptr[0]=i+2;
    int j;
    for(j=i-1,i=4;j>i;j--,i++){
    	wordptr[j]=wordptr[i]^wordptr[j];
    	wordptr[i]=wordptr[i]^wordptr[j];
    	wordptr[j]=wordptr[i]^wordptr[j];

    }
    brushstring((ox*thetime/STEPS)+(ox1*(STEPS-thetime)/STEPS),(oy*thetime/STEPS)+(oy1*(STEPS-thetime)/STEPS),wordptr,50,2,&buffer,0xFF,0x88,0xFF);
    //LOGE("brushlihua");
    //brushlihua(&buffer);
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
    	if (AMotionEvent_getAction(event)==AMOTION_EVENT_ACTION_UP){
    		pausemakevalue=0;
    	}
    	if (AMotionEvent_getAction(event)==AMOTION_EVENT_ACTION_DOWN){
    		pausemakevalue=1;
    	}
        engine->animating = 1;
        //LOGI("%d",AMotionEvent_getPointerCount(event));
        return 1;
    } else if (AInputEvent_getType(event) == AINPUT_EVENT_TYPE_KEY) {
//        LOGI("Key event: action=%d keyCode=%d metaState=0x%x",
//                AKeyEvent_getAction(event),
//                AKeyEvent_getKeyCode(event),
//                AKeyEvent_getMetaState(event));
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
                engine->animating=1;
            //    setPlayingAssetAudioPlayer(1);
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
           // setPlayingAssetAudioPlayer(1);
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
        	Truning=0;
        	pthread_join(pt,NULL);
        //    shutdown();
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

  //  createEngine();
  //  createAssetAudioPlayer(engine.app->activity->assetManager,"2.mp3");
    inittv();
	kind=0;
	brushvalue=1;
	stopkind=0;
	thisvalue=1;
	Truning=1;
	pausemakevalue=0;
	pthread_create(&pt, NULL, &makevalue, NULL);
	//kind=getkind();

	//stopkind=1;
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
//                        LOGI("accelerometer: x=%f y=%f z=%f",
//                                event.acceleration.x, event.acceleration.y,
//                                event.acceleration.z);
                    }
                }
            }

            // Check if we are exiting.
            if (state->destroyRequested != 0) {
                engine_term_display(&engine);
                return;
            }
        }

        //makevalue();
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
