
#include <sys/stat.h>
#include <sys/param.h>

#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/rand.h>
#include <openssl/bio.h>

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>

#include <getopt.h>
#include <glib.h>
#include <glib/gstdio.h>

#include <ccnet/option.h>

#include "rsa.h"
#include "utils.h"

enum {
    ERR_NAME_NULL = 1,
    ERR_NAME_INVALID,
    ERR_PERMISSION,
    ERR_CONF_FILE,
};

/* Number of bits in the RSA/DSA key.  This value can be set on the command line. */
#define DEFAULT_BITS        2048
static guint32 bits = 0;

static int quiet = 0;

static char *identity_file_peer = NULL;

static RSA *peer_privkey = NULL;
static RSA *peer_pubkey = NULL;

static char *user_name = NULL;
static char *peer_name = NULL;
static char *peer_id = NULL;
static char *host_str = NULL;
static char *port_str = NULL;

/* argv0 */
static char *program_name = NULL;


static void make_configure_file (const char *config_file);

void
save_privkey (RSA *key, const char *file)
{
    FILE *f;
    f = g_fopen (file, "wb");
    PEM_write_RSAPrivateKey(f, key, NULL, NULL, 0, NULL, NULL);
    fclose (f);
}

static void
create_peerkey ()
{
    peer_privkey = generate_private_key (bits);
    peer_pubkey = private_key_to_pub (peer_privkey);
}


static const char *short_opts = "hc:n:H:P:F:";
static const struct option long_opts[] = {
    { "help", no_argument, NULL, 'h' },
    { "config-dir", required_argument, NULL, 'c' },
    { "central-config-dir", required_argument, NULL, 'F' },
    { "name", required_argument, NULL, 'n' },
    { "host", required_argument, NULL, 'H' },
    { "port", required_argument, NULL, 'P' },
    { 0, 0, 0, 0 },
};

void usage (int exit_status)
{
    printf ("Usage: %s [OPTION]...\n", program_name);

    fputs ("Init ccnet configuration directory.\n\n", stdout);

    fputs ("Mandatory arguments to long options are mandatory "
           "for short options too.\n", stdout);

    fputs (""
"  -c, --config-dir=DIR      use DIR as the output ccnet configuration\n"
"                              directory. Default is ~/.ccnet\n"
           , stdout);
    fputs (""
"  -n, --name=NAME           your public name\n"
           , stdout);
    fputs (""
"  -H, --host=<ip or domain> Public addr. Only useful for server.\n"
           , stdout);
    fputs (""
"  -P, --port=port           Public port. Only useful for server.\n"
"                            Default 10001.\n"
           , stdout);

    exit (exit_status);
}

static int
is_valid_username (const char *name)
{
    const char *p = name;
    while (*p) {
        if (!isalnum(*p) && *p != '_' && *p != '-')
            return 0;
        ++p;
    }

    return 1;
}

int
main(int argc, char **argv)
{
    char *config_dir;
    char *central_config_dir = NULL;
    char *config_file;
    int c;
    char *name = NULL;

    program_name = argv[0];

    config_dir = DEFAULT_CONFIG_DIR;

    while ((c = getopt_long(argc, argv,
        short_opts, long_opts, NULL)) != EOF) {
        switch (c) {
        case 'h':
            usage (1);
            break;
        case 'F':
            central_config_dir = strdup(optarg);
            break;
        case 'c':
            config_dir = strdup(optarg);
            break;
        case 'n':
            name = strdup (optarg);
            break;
        case 'H':
            host_str = strdup (optarg);
            break;
        case 'P':
            port_str = strdup (optarg);
            break;
        default:
            usage(1);
        }
    }

    config_dir = ccnet_expand_path (config_dir);
    /* printf("[conf_dir=%s\n]", config_dir); */
    SSLeay_add_all_algorithms();

    if (RAND_status() != 1) {   /* it should be seeded automatically */
        fprintf(stderr, "PRNG is not seeded\n");
        exit (1);
    }

    if (bits == 0)
        bits = DEFAULT_BITS;

    /* create peer key */
    if (!name) {
        usage(-ERR_NAME_NULL);
    }
    if (strlen(name) < 2 || strlen (name) > 16
        || !is_valid_username(name)) {
        fprintf (stderr, "The user name should be more than 2 bytes and less than 16 bytes, only digits,  alphabetes and '-', '_' are allowed");
        exit(-ERR_NAME_INVALID);
    }

    user_name = name;
    peer_name = g_strdup (name);

    create_peerkey ();
    peer_id = id_from_pubkey (peer_pubkey);
    identity_file_peer = g_build_filename (config_dir, PEER_KEYFILE, NULL);

    /* create dir */
    if (ccnet_mkdir(config_dir, 0700) < 0) {
        fprintf (stderr, "Make dir %s error: %s\n",
                 config_dir, strerror(errno));
        exit(-ERR_PERMISSION);
    }

    struct stat st;
    if (central_config_dir && g_stat(central_config_dir, &st) < 0 &&
        ccnet_mkdir(central_config_dir, 0700) < 0) {
        fprintf(stderr, "Make dir %s error: %s\n", central_config_dir,
                strerror(errno));
        exit(-ERR_PERMISSION);
    }

    /* save key */
    save_privkey (peer_privkey, identity_file_peer);

    /* make configure file */
    config_file = g_build_filename (central_config_dir ? central_config_dir : config_dir, CONFIG_FILE_NAME, NULL);
    make_configure_file (config_file);

    printf ("Successly create configuration dir %s.\n", config_dir);
    exit(0);
}


static void
make_configure_file (const char *config_file)
{
    FILE *fp;

    if ((fp = g_fopen(config_file, "wb")) == NULL) {
        fprintf (stderr, "Open config file %s error: %s\n",
                 config_file, strerror(errno));
        exit(-ERR_CONF_FILE);
    }

    fprintf (fp, "[General]\n");
    fprintf (fp, "USER_NAME = %s\n", user_name);
    fprintf (fp, "ID = %s\n", peer_id);
    fprintf (fp, "NAME = %s\n", peer_name);
    if (host_str)
        fprintf (fp, "SERVICE_URL = http://%s:8000\n", host_str);

    if (port_str) {
        fprintf (fp, "\n");
        fprintf (fp, "[Network]\n");
        fprintf (fp, "PORT = %s\n", port_str);
    }

    fprintf (fp, "\n");
    fprintf (fp, "[Client]\n");

    /* Use differnet ports for ccnet-daemon and ccnet-server */
    if (port_str != NULL) {
        /* ccnet-server */
        fprintf (fp, "PORT = 13418\n");
    } else {
        /* ccnet-daemon */
        fprintf (fp, "PORT = 13419\n");
    }

    fclose (fp);

    fprintf (stdout, "done\n");
}
