#include    <stdio.h>
#include    <sys/types.h>
#include    <sys/socket.h>
#include    <netinet/in.h>

#include    <arpa/inet.h>
#include    <unistd.h> // for close()
#include    <string.h>
#include    <stdint.h>
#include    <sys/stat.h>
#include    <getopt.h>
#include    <stdbool.h>
#include    <errno.h>

#include    <stdlib.h>

/*
 *   Written by Michael G. Katzmann (vk2bea@gmail.com)
 *
 *   Program to transfer a compiled inverse assembler relocatable file to an HP logic analyzer.
 *
 *   The HP 'ASM' program (from the HP 10391B Inverse Assembler Development Package) will produce
 *   a '.R' (relocatable) file but this cannot be used directly on the logic analyzer.
 *   The final linked program file is created using the 'IALDOWN' program (from the HP 10391B
 *   Inverse Assembler Development Package) in transferring the .R file vi RS-232.
 *   It will also be created when the .R file is transferred to the logic analyzer using GPIB
 *   or telnet with the ":MMEMory:DOWNload" command.
 *
 *   The "HP 10391B Inverse Assembler Development Package", which includes the ASM program
 *   is available from Keysight at:
 *   https://www.keysight.com/us/en/lib/software-detail/instrument-firmware-software/10391b-inverse-assembler-development-package-version-0200-sw575.html
 *   The ASM program (inverse assembler compiler) will run under 'dosbox' on Linux.
 *   $ sudo dnf install dosbox
 *
 *   This program transfers the file using telnet.
 *
 *   See P542 of HP 1660E/ES/EP and 1670E Series Logic Analyzer User's Guide (Publication 01660-97028)
 *   on connecting to the telnet port of the logic analyzer.
 *
 *   Compile with:      $ gcc -o IAdown IAdown.c
 *   Example execution: $ IAdown -a 192.168.1.16 -n I6809 -d \"MC6809 Inverse Assembler\" I6890.R
 */

#define TELNET_PORT 5025
#define LA_HOST_ADDRESS "192.168.1.16"

#define STRING_BUFSIZE 100
#define FILE_BUFSIZE 10000

void
usage( char* progName ) {
    fprintf( stderr, "Usage: %s -a IP_ADDRESS -f IA_FILE [-r] [-d \"Description\"] IA_FILE.R\n"
    "       -a | --address       IP address of HP logic analyzer\n"
    "       -n | --name          File name on logic analyzer\n"
    "       -d | --description   Descriptive string for the inverse assembler\n"
    "       -f | --floppy        Create the file on the floppy drive\n"
    "            --verbose       Show debugging information\n\n"

    "e.g.: %s -a 192.168.1.16 -n I6809 -d \"MC6809 Inverse Assembler\" I6890.R\n\n"
    "The file name (-n) can be up to 11 characters for LIF (NNNNNNNNNN)\nor 12 for DOS (NNNNNNNN.NNN).\n"
    "The maximum length of the description (-d) string is 32 characters.\n", progName, progName );
}

int
main(int argc, char *argv[])
{
    int sockfd;					        // IP socket file descriptor
    struct sockaddr_in serv_addr;		// IP socket address structure

    char *sIPaddr              = NULL,  // IP address string (e.g. 192.168.1.16)
         *sFileName            = NULL,  // file name on the logic analyzer
         *sFileDescription     = "Inverse Assembler",
         *sSourceFileName      = NULL,  // .R file (re-locatable inverse assembler)
         *sIDquery             = "*IDN?\r\n";

    static int  bVerbose = 0,			// set if additional information is to be displayed
                bFloppy  = 0;           // set if destination file to be stored on floppy

    struct stat sb;                     // Structure used to obtain the size of the source re-locatable inverse assembler file

#define FILE_BUFSIZE 10000
    unsigned char buffer[ FILE_BUFSIZE ];   // buffer for reading in .R binary file
    char stringBuffer[ STRING_BUFSIZE ];    // buffer for receiving ID string fro logic analyzer

    FILE *filePtr;                      // file structure obtained when opening re-locatable IA file
    size_t bytesRead, totalBytes = 0;   // number of bytes

    struct timeval timeout = { 0 };     // timeout for IP communication

    int opt;
    int option_index = 0;

    char *dotPosn;

    while (true) {
        static struct option long_options[] = {
            {"verbose",     no_argument,       &bVerbose, true},
            {"floppy",      no_argument,       0,         'f' },
            {"description", required_argument, 0,         'd' },
            {"address",     required_argument, 0,         'a' },
            {"name",        required_argument, 0,         'n' },
            {0, 0, 0, 0}
        };

      opt = getopt_long (argc, argv, "a:n:d:f",
                       long_options, &option_index);

      // Detect the end of the options.
    if (opt == -1)
        break;

    switch (opt) {
        case 0:
            break;

        case 'f':  // Use floppy rather than hard internal disc as destination
            bFloppy = true;
            break;

        case 'a': // IP address of logic analyzer
            sIPaddr = optarg;
            break;

        case 'd': // description of file
            sFileDescription = optarg;
            if( strlen( sFileDescription ) > 32 ) {
                fprintf( stderr, "error: -d argument too long\n");
                exit(EXIT_FAILURE);
            }
            break;

        case 'n': // name of destination file
            sFileName = optarg;
            dotPosn = strchr(sFileName, '.');
            if( (dotPosn != NULL && dotPosn - sFileName > 8)
                    || (dotPosn == NULL && strlen( sFileName ) > 11 )
                    || strlen( sFileName ) > 12 ) {
                fprintf( stderr, "error: -f bad filename argument\n");
                exit(EXIT_FAILURE);
          }
          break;

        case '?':
            usage( argv[0] );
            exit(EXIT_FAILURE);
          break;

        default:
            exit(EXIT_FAILURE);
        }
    }

    // We require the user provide the source file name as an argument
    if( argc == optind ) {
        fprintf( stderr, "error: missing inverse assembler file (.R).\n");
        usage( argv[0] );
        exit(EXIT_FAILURE);
    } else if( argc > optind + 1 ) {
        fprintf( stderr, "error: too many arguments.\n");
        usage( argv[0] );
        exit(EXIT_FAILURE);
    } else {
        sSourceFileName = argv[optind];
    }

    // At minimum we need the name of the destination file and the IP address of the logic analyzer
    if( sIPaddr == NULL || sFileName == NULL ) {
        fprintf( stderr, "IP address of logic analyzer and filename of inverse assembler (.r file) required\n");
        usage( argv[0] );
        exit(EXIT_FAILURE);
    }

    if( bVerbose ) {
        fprintf( stderr, "\n"
            " IP address: %s port %d\n"
            " Local file: %s\n"
            "Remote file: %s (on %s disk)\n"
            "Description: %s\n\n",
            sIPaddr, TELNET_PORT, sSourceFileName, sFileName, bFloppy ? "floppy" : "hard", sFileDescription );
    }

    // get the size of the input binary re-locatable inverse assembler file; this is needed for the header
    if (lstat( sSourceFileName, &sb) == -1) {
        fprintf( stderr, "Cannot find / open file: %s\n", sSourceFileName );
        exit(EXIT_FAILURE);
    } else if ( bVerbose ) {
        fprintf( stderr, "Input file %s is %jd bytes in size\n", sSourceFileName, (intmax_t) sb.st_size);
    }

    // Initialize the server socket
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr ( sIPaddr );
    serv_addr.sin_port = htons ( TELNET_PORT );

    // Create an endpoint for communication with the logic analyzer
    sockfd = socket( AF_INET, SOCK_STREAM, 0 );

    // Set the timeout for IP communication to 10 seconds
    timeout.tv_sec  = 10;
    timeout.tv_usec = 0;

    setsockopt (sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof timeout);
    setsockopt (sockfd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof timeout);

    // Initiate a connection on the created socket
    if( connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0 ) {
        fprintf(stderr, "Connection error to %s: %s\n", sIPaddr, strerror(errno));
        exit(EXIT_FAILURE);
    }

    // Ask for the ID string ... this confirms good communication

    // Receive a message from the 1660ES socket
    if( (send( sockfd, sIDquery, strlen ( sIDquery ), 0 ) < 0) ||
        (recv( sockfd, stringBuffer, STRING_BUFSIZE, 0 ) < 0) ) {
        fprintf( stderr, "Error checking *IDN? from logic analyzer: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    } else {
        printf ( "%s\n", stringBuffer );
    }

    // See page 158 of HP 1660E/ES/EP-Series Logic Analyzers programmer's guide (Publication number 01660-97029)
    snprintf( stringBuffer, STRING_BUFSIZE, ":mmemory:download '%s',internal%1d,'%s',-15614,#8%08jd%c",
            sFileName,
            bFloppy ? 1 : 0,
            sFileDescription,
            sb.st_size + 1,     // +1 accounts for the NULL that precedes the file contents
            0 );   // This NULL is equivalent to the 'B' Invasm field option in the IALDOWN program
                   // The "Invasm" field is used for microprocessors with limited status information.
                   // For more information on using this field, see appendix B. of
                   // "HP 10391B Inverse Assembler Development Package Reference Manual" Part No. 10391-90903

    if( bVerbose ) {
        fprintf( stderr, "header: \"" );
        fwrite( stringBuffer, sizeof( char ), strlen ( stringBuffer ) + 1, stderr );    // +1 outputs the trailing null
        fprintf( stderr, "\"\n" );
    }

    // Send the header on the created socket
    // also send the trailing NULL (the +1) representing the invasm field option
    if( send ( sockfd, stringBuffer, strlen ( stringBuffer ) + 1, 0 ) < 0 ) {           // +1 outputs the trailing null
        fprintf( stderr, "Error sending data: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    // Open the source relocatable inverse assembler (.R) file
    if( (filePtr = fopen( sSourceFileName,"rb")) == NULL ) {  // r for read, b for binary
        fprintf( stderr, "error: %s\n", strerror( errno ) );
        exit(EXIT_FAILURE);
    }
    // Read the contents in chunks and send it over the socket to the logic analyzer
    do {
        bytesRead = fread(buffer, sizeof( unsigned char),FILE_BUFSIZE, filePtr);
        totalBytes += bytesRead;
        if( send ( sockfd, buffer, bytesRead, 0 ) < 0 ) {
            fprintf( stderr, "Error sending file data: %s\n", strerror(errno));
            exit(EXIT_FAILURE);
        }
    } while( bytesRead == FILE_BUFSIZE );

    // Send a final line feed
    if( send ( sockfd, "\n", 1, 0 ) < 0 ) {
        fprintf( stderr, "Error trailing LF: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    // We should have read and sent the same number of bytes as we determined were in the file
    if( totalBytes != sb.st_size ) {
        fprintf( stderr, "Short file read: %jd of %jd bytes\n", totalBytes, sb.st_size );
        fclose( filePtr );
        exit(EXIT_FAILURE);
    }

    // Clean-up
    close ( sockfd );
    fclose( filePtr );

    return EXIT_SUCCESS;
}
