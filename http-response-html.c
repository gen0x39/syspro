
/*
 * http-response-html.c -- HTTPの応答を画面に表示する(.html専用、枠組みだけ)
 * ~yas/syspro/ipc/http-response-html.c
 */

#include <stdio.h>  /* fprintf() */
#include <stdlib.h> /* exit() */
#include <string.h> /* strrchr() */
#define BUFFERSIZE 100

extern void http_send_reply( FILE *out, char *filename );
extern void http_send_reply_bad_request( FILE *out );
extern void http_send_reply_not_found( FILE *out );

int
main( int argc, char *argv[] )
{
	char *filename;

	if( argc != 2 ) {
		fprintf(stderr,"Usage: %s filename\n",argv[0] );
		exit( 1 );
	}
	filename = argv[1];
	// 1. main()はargv[1]とstdoutを引数に，http_send_reply()を呼ぶ
	http_send_reply( stdout, filename );
}

void
http_send_reply( FILE *out, char *filename )
{
	char *ext;

	// 2. 拡張子の取得
	ext = strrchr( filename, '.' );
	if( ext == NULL )
	{
		http_send_reply_bad_request( out );
		return;
	}
	// 3. 拡張子が.htmlの時のみ処理
	else if( strcmp( ext,".html" ) == 0 )
	{
		printf("filename is [%s], and extention is [%s].\n", 
		       filename, ext);

		// 4. 実際のファイル名の作成
		char buf[BUFFERSIZE];
		snprintf(buf, BUFFERSIZE, "./%s", filename);
		printf("buf : [%s]\n",buf);

		// 5. fopenでファイルを開く，失敗したらhttp_send_reply_not_found()を送る
		FILE *fp;
		fp = fopen(buf,"r");
		if(fp == NULL) {
			http_send_reply_not_found( out );
			exit(1);
		}
		// 6.7.8. ファイルを開くのに成功したのでstatus lineを送る
		else {
			fprintf(out,"HTTP/1.0 200 OK\r\n");
			fprintf(out,"Content-Type: text/html\r\n");
			fprintf(out,"Content-Type: text/html\r\n");

			// 9. ファイルの内容をfgets()とfprintf()で出力
			char rheader[BUFFERSIZE];
			while (fgets(rheader, BUFFERSIZE, fp)) {
				fprintf(out, "%s", rheader);
			}

			// 10. ファイルを閉じる
			fclose(fp);
		}
		return;
	}
	else
	{
		http_send_reply_bad_request( out );
		return;
	}
}

void
http_send_reply_bad_request( FILE *out )
{
	fprintf(out,"HTTP/1.0 400 Bad Request\r\nContent-Type: text/html\r\n\r\n");
    	fprintf(out,"<html><head></head><body>400 Bad Request</body></html>\n");
}

void
http_send_reply_not_found( FILE *out )
{
	fprintf(out,"HTTP/1.0 404 Not Found\r\nContent-Type: text/html\r\n\r\n");
    	fprintf(out,"<html><head></head><body>404 Not Found</body></html>\n");
}
