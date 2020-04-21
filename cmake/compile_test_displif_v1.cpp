#include <stdint.h>

extern "C" {
#include <xen/io/displif.h>
}

/**
 * Older displif protocol header had its version defined as a string,
 * so we need a check if we can safely test protocol version with a cpp
 * preprocessor: if the below fails it means that XENDISPL_PROTOCOL_VERSION
 * is defined as a string "1".
 */
int main(int argc, char **argv)
{
#if XENDISPL_PROTOCOL_VERSION >= 2
#endif
	return 0;
}
