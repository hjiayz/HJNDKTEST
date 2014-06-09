#include <stdio.h>
#include <math.h>
        char tv[16][3]={
                        {1,3},//2
                        {3,1},//2
                        {4,6},//5
                        {6,4},//5
                        {7,9},//8
                        {9,7},//8
                        {1,7},//4
                        {7,1},//4
                        {2,8},//5
                        {8,2},//5
                        {3,9},//6
                        {9,3},//6
                        {1,9},//5
                        {9,1},//5
                        {3,7},//5
                        {7,3}//5
        };
//测试值,如果真 返回 -10 否则返回错误值的位置.
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
	//检测不存在路径,即直线上有其他点的路径,由于手机只要直线上的其他点已经激活,这样的路径也会被激活,所以排除掉已经有点被激活的情况.例如125364是被允许的,因为虽然64要穿过5,但是5已经存在.
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
//剪枝
int cut(int a,int b){
	int p=(int)pow(10,b);
	return a-a%p+(int)pow(10,b);
}
int getkind(){
	int i,count=0,t;
	for (i=1;i<1000000000;){
//		printf("%d\n",i);
		t=testvalue(i);
		if (t==-10) {
			count++;
			//如果要输出正解,去掉下面的注释
			//printf("%d\n",i);
			i++;
		}
		else {
			i=cut(i,t);
		//	i++;
		}
		
	}
/*穷举
	printf("%d\n",count);
	count=0;
        for (i=1;i<1000000000;){
//              printf("%d\n",i);
                t=testvalue(i);
                if (t==-10) {
                        count++;
                        //printf("%d\n",i);
                        i++;
                }
                else {
                //        i=cut(i,t);
                      i++;
                }

        }
*/
	return count;
}
void init(){
	int i;
	for (i=0;i<16;i++){
		tv[i][2]=(tv[i][0]+tv[i][1])/2;
	}
}
int main(){
	init();
	printf("%d\n",getkind());
//	printf("%d\n",testvalue(125637));
	return 0;
}
