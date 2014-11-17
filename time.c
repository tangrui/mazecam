#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <assert.h>
#include <sys/time.h>
#include <limits.h>
#include <unistd.h>

int main()
{
    // char str[30];
    // time_t t1;
    // struct tm *tm;
    // t1 = time(NULL);
    // tm = localtime(&t1);
    // sprintf(str,"%d-%d-%d-%d-%d-%d",tm->tm_year+1900,tm->tm_mon+1,tm->tm_mday,tm->tm_hour,tm->tm_min,tm->tm_sec);
    // printf("%s\n",str);
    // printf("%zu\n", sizeof(int) );
    // printf("%d\n", INT_MAX);



    int count = 1;
	char picture_name[50];

    while(count > 0)
    {
	    struct timeval  tv;
	    gettimeofday(&tv, NULL);

	    unsigned long time_in_mill = (tv.tv_sec) * 1000 + (tv.tv_usec) / 1000 ;

	    // sprintf(picture_name, "zyeeda-%d.jpg", time_in_mill);
	    // printf("string is : %d\n", picture_name);
	    printf("double is : %lu\n", time_in_mill);

		unsigned long long millisecondsSinceEpoch =
		    (unsigned long long)(tv.tv_sec) * 1000 +
		    (unsigned long long)(tv.tv_usec) / 1000;

		printf("llu : %llu\n", millisecondsSinceEpoch);
    }

    return 1;
}