/*
 * @Author: your name
 * @Date: 2020-05-06 09:42:46
 * @LastEditTime: 2020-05-06 09:44:48
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: /AiThinkerProjectForESP/Ai-examples/components/common/common.c
 */

#include "common.h"

// 将字符数组 s 颠倒
void reverse(char *s, int length)
{
    int mid = length / 2;
    for (int i = 0; i < mid; i++)
    {
        char t = s[i];
        s[i] = s[length - i - 1];
        s[length - i - 1] = t;
    }
}
// 计算整数 n 的位数
int count(int n)
{
    int t = 0;
    do
    {
        n /= 10;
        t++;
    } while (n > 0);
    return t;
}
char *itoaa(int n) // 1234
{
    int num = count(n);
    char *chs = (char *)malloc(sizeof(char) * (num + 1));
    int i = 0, t;
    do
    {
        t = n % 10;
        chs[i++] = t + '0';
        n /= 10;
    } while (n > 0);
    reverse(chs, num); // 逆转，将 4321 逆转成 1234
    chs[num] = '\0';
    return chs;
}
