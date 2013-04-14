#include <arpa/inet.h>

#include "network_p.h"

/* サーバに接続 */
void connect_to_server( const char* const addr ) {
  int sock;                      // ソケットファイルディスクリプタ
  struct sockaddr_in server;     // 接続先ホスト

  /* ソケットの作成 */
  sock = socket( PF_INET, SOCK_STREAM, 0 );
  if ( sock == -1 ) {     // 作成失敗
    perror("error: sock");
    exit(1);
  }

  /* 接続先ホストの設定 */
  memset( &server, 0, sizeof(server) );              // ゼロクリア
  server.sin_family = PF_INET;                       // プロトコル: IPv4
  server.sin_port = htons(50000);                    // ポート番号: 50000
  server.sin_addr.s_addr = inet_addr(addr);   // IPアドレス: 127.0.0.1

  /* サーバに接続 */
  if ( connect( sock, (const struct sockaddr *)&server, sizeof(server) ) != 0 ) {    // 接続失敗
    perror("error:");
    close(sock);
    exit(1);
  }

  /* コマンドを実行 */
  sent_to_server( sock );
  
  close(sock);    // ソケットを閉じる
}

/* クライアントからの接続を待ち受ける */
void connect_from_client( ) {
  int sock, fd;                 // ソケットファイルディスクリプタ
  struct sockaddr_in local;     // 自ホスト

  /* ソケットの作成 */
  sock = socket( PF_INET, SOCK_STREAM, 0 );
  if ( sock == -1 ) {     // 作成失敗
    perror("error: sock");
    exit(1);
  }

  /* 自ホストの設定 */
  memset( &local, 0, sizeof(local) );           // ゼロクリア
  local.sin_family = PF_INET;                   // プロトコル: IPv4
  local.sin_port = htons(50000);                // ポート番号: 50000
  local.sin_addr.s_addr = htonl(INADDR_ANY);    // IPアドレス: 192.168.0.1

  /* バインド処理  */
  if ( bind( sock, (const struct sockaddr *)&local, sizeof(local) ) != 0  ) {     // バインド失敗
    perror("error:");
    close(sock);
    exit(1);
  }

  /* 接続待ち受けの準備 */
  if ( listen( sock, 5 ) != 0 ) {    // 待ち受け準備失敗
    perror("error:");
    close(sock);
    exit(1);
  }

  struct sockaddr_in client;    // 接続元ホスト
  socklen_t len;                // 構造体のサイズ
  while ( 1 ) {
    len = sizeof(client);
    fd = accept( sock, (struct sockaddr*)&client, &len );    // 接続待ち受け
    
    if ( fd < 0 ) {    // 接続失敗
      perror("error: fd");
    }
    else {             // 接続成功
      recv_from_server( fd );
      close(fd);    // ソケットを閉じる
    }
  }
  
  close(sock);    // ソケットを閉じる
}

void sent_to_server( int sock ) {
  char str[BUFFERSIZE];
  ssize_t len;
  while ( 1 ) {
    /* プロンプトを受信*/
    len = read( sock, str, sizeof(str) - 1 );
    if ( len > 0 ) {    // 受信成功
      str[len] = '\0';
      printf("%s ", str);
    }
    else {    // 受信失敗
      perror("error: prompt");
      break;
    }

    /* コマンドの入力 */
    fgets( str, sizeof(str) - 1, stdin );
    
    /* コマンドを送信 */
    len = write( sock, str, strlen(str) );
    if ( len == -1 ) {    // 送信失敗
      perror("error: command");
      break;
    }

    if ( strcmp( str, "exit" ) == 0 ) {
      break;
    }
    
    /* 結果を受信 */
    while ( 1 ) {
      len = read( sock, str, sizeof(str) - 1 );
      if ( len > 0 ) {    // 受信成功
        str[len] = '\0';

        if ( strcmp( str, "quit: result" ) == 0 ) {
          break;
        }
      }
      else {    // 受信失敗
        perror("error: prompt");
        goto END1;
      }

      /* 結果の表示 */
      printf("%s", str);
      
      /* 返信の送信 */
      if ( receipt_confirmation( sock, SEND ) == ERROR ) {
        goto END1;
      }
    }

    /* 返信の送信 */
    if ( receipt_confirmation( sock, SEND ) == ERROR ) {
      break;
    }
  }

 END1:
  ;
}

void recv_from_server( int fd ) {
  char str[BUFFERSIZE];
  const char prompt[] = ">";
  const char quit_str[] = "quit: result";  // 終了文字
  ssize_t len;
  while ( 1 ) {
    /* プロンプトを送信 */
    len = write( fd, prompt, strlen(prompt) );
    if ( len == -1 ) {    // 送信失敗
      perror("error: prompt");
      break;
    }
    
    /* コマンドを受信 */
    len = read( fd, str, sizeof(str) - 1 );
    if ( len > 0 ) {    // 受信成功
      str[len] = '\0';
    }
    else {    // 受信失敗
      perror("error: command");
      break;
    }

    if ( strcmp( str, "exit" ) == 0 ) {
      break;
    }
    
    /* コマンドの実行 */
    FILE *fp;
    fp = popen( str, "r" );
    if ( fp ) {
      /* 結果を送信 */
      while( fgets( str, BUFFERSIZE, fp ) != NULL ) { 
        len = write( fd, str, sizeof(str) - 1 );
        if ( len == -1 ) {    // 送信失敗
          perror("error: prompt");
          pclose(fp);
          goto END2;
        }
        
        /* 返信の受信 */
        if ( receipt_confirmation( fd, RECV ) == ERROR ) {
          pclose(fp);
          goto END2;
        }      
      }
    }
  
    /* 終了文字列を送信 */
    len = write( fd, quit_str, strlen(quit_str) );
    if ( len == -1 ) {    // 送信失敗
      perror("error: quit len");
      pclose(fp);
      break;
    }

    /* 返信の受信 */
    if ( receipt_confirmation( fd, RECV ) == ERROR ) {
      pclose(fp);
      break;
    }
    
    pclose(fp);
  }

 END2:
  ;
}

/* 返信文字の送受信 */
int receipt_confirmation( int sock, int check ) {
  ssize_t len;

  if ( check == RECV ) {    // 返信文字の受信
    char str[3];

    /* 受信処理 */
    len = read( sock, str, sizeof(str) - 1 );
    if ( len > 0 ) {    // 受信成功
      str[len] = '\0';

      /* 返信文字の確認 */
      if ( strcmp( str, "OK" ) ) {
        return ERROR;
      }
    }
    else {    // 受信失敗
      perror("error: OK");
      return ERROR;
    }
  }
  else {    // 返信文字の送信
    char str[] = "OK";

    /* 送信処理 */
    len = write( sock, str, strlen(str) );
    if ( len == -1 ) {    // 送信失敗
      perror("error: OK");
      return ERROR;
    }
  }
    
  return SUCCESS;
}
