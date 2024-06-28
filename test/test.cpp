#include "bits/stdc++.h"
using namespace std;
using ll = long long;
int rand7() {
	// 生成 0~6 的均匀随机数。
}
int rand10(int n = 10) { // 0 ~ n-1
	int num = 1;
	ll cur = 7;
	while (cur < n) {
		num++;
		cur *= 7;
	}
	// cur = 49, num = 2
	const ll mx = (cur / n) * n; // 去除那些导致不均匀的
	// mx = 40   分组映射  0~9 10~19 20~29 30~39  分别映射到 0~9
	// 0 ~ mx-1 映射到 0 ~ n-1
	while (1) {
		ll ret = 0;
		
		// 构造一个二进制 7 位数 ret。
		for (int i = 0; i < num; i++) {
			ret = ret * 7 + rand7();
		}
		
		if (ret < mx) // 40 ~ 48 的情况，不成完整一组。重新来过。
			return ret % n;
	}
	
	// 概率（while 平均循环几次）。
	// 1次  while  40/49
	// 2次  while  (1-40/49)*(40/49)
	// 3次  while  ((1-40/49)**(3-1))*(40/49)
	// 4次  while  ((1-40/49)**(4-1))*(40/49)
	// ...
	

	// 为什么要这么做，为什么不两个 rand7 加起来。
	
	return -1;
}

int main() {
	
	srand(time(0));

	return 0;
}