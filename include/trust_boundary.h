#ifndef TRUST_BOUNDARY_H
#define TRUST_BOUNDARY_H

/**
 * TRUST_BOUNDARY — mark a function as a trust-boundary interface.
 *
 * Place this on any function declaration whose by-value record arguments or
 * return value cross a trust domain.  The security-misc-padding-boundary-leak
 * checker uses this annotation to instantiate the boundary set B and extract
 * boundary-transfer events EB (Chapters 6–7 of the thesis).
 *
 * Usage:
 *   TRUST_BOUNDARY int send_packet(struct Packet pkt);
 *   TRUST_BOUNDARY struct Reply receive_reply(void);
 *
 * The annotation is a no-op for all other compilers/tools.
 */
#if defined(__clang__) || defined(__GNUC__)
#  define TRUST_BOUNDARY __attribute__((annotate("trust_boundary")))
#else
#  define TRUST_BOUNDARY
#endif

#endif /* TRUST_BOUNDARY_H */
