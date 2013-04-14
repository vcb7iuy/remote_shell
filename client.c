#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "network.h"

int main( int argc, char** argv ) {
  /* 引数の確認 */
  if ( argc != 2 ) {
    fprintf( stderr, "Usege: <プログラム名> <IPアドレス>\n");
    exit(1);
  }

  /* IPアドレスの確認 */
  if ( strlen(argv[1]) + 1 >= BUFFERSIZE ) {
    fprintf( stderr, "error: IPaddress length\n");
    exit(1);
  }

  /* サーバに接続 */
  connect_to_server( argv[1] );

  return 0;
}


