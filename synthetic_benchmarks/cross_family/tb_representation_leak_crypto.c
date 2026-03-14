// tb_representation_leak_crypto.c
//
// Cross-family synthetic benchmark: DM (data-movement representation) + CM
// (cryptographic misuse) — thesis taxonomy §5.2, §5.3, §5.5 rule DM-10.
//
// Scenario:
//   A HandshakeState struct is built field-by-field (padding indeterminate),
//   then its in-memory image is passed to a digest function before being
//   sent across a trust boundary by value.
//
//   This illustrates two overlapping misconfigurations:
//   (1) DM-10: a cryptographic digest is computed over (&s, sizeof(s)) where
//       s has padding — the tag becomes a function of indeterminate padding
//       bytes, making it nondeterministic and unreproducible.
//   (2) TB-DL2: the struct is transferred by value across a trust boundary,
//       exposing both semantic fields and padding bytes to the receiver.
//
// Expected checker behaviour:
//   - security-misc-padding-boundary-leak fires on the send_to_untrusted()
//     call because HandshakeState has padding and no whole-object init is
//     visible before the call.
//
// Note: uses the same TRUST_BOUNDARY annotation as the main benchmark suite
// so the checker recognises the boundary correctly.

#include "../../include/trust_boundary.h"

#include <stdint.h>
#include <stddef.h>
#include <string.h>

// ---------------------------------------------------------------------------
// Stub digest API — stands in for a real cryptographic hash.
// Consumes exactly the bytes it is given, padding included.
// ---------------------------------------------------------------------------
static void digest32(const void *data, size_t n, uint8_t out[32]) {
    const uint8_t *p = (const uint8_t *)data;
    uint8_t acc = 0;
    for (size_t i = 0; i < n; ++i) acc ^= p[i];
    for (int i = 0; i < 32; ++i) out[i] = (uint8_t)(acc + i);
}

// ---------------------------------------------------------------------------
// HandshakeState layout (x86-64 SysV ABI):
//   offset  0: version  (1 byte)
//   offset  1: [7 bytes padding — aligning nonce to 8-byte boundary]
//   offset  8: nonce    (8 bytes)
//   offset 16: msg_len  (4 bytes)
//   offset 20: [4 bytes tail padding — to make sizeof = 40... actually
//               tag starts at 20, sizeof = 20+16 = 36, padded to 40]
//   offset 20: tag      (16 bytes)
//   total size: 40 bytes, ~11 bytes padding
//
// HasPadding(HandshakeState) = true
// ---------------------------------------------------------------------------
typedef struct {
    uint8_t  version;   /* 1 byte  */
    /* ABI inserts ~7 bytes padding here to align nonce to 8 bytes */
    uint64_t nonce;     /* 8 bytes */
    uint32_t msg_len;   /* 4 bytes */
    uint8_t  tag[16];   /* 16 bytes — pretend MAC/authentication tag */
} HandshakeState;

// ---------------------------------------------------------------------------
// Trust boundary — annotated with TRUST_BOUNDARY so the checker extracts
// a call-by-value boundary-transfer event when this is called.
// This represents an IPC or RPC boundary to an untrusted component.
// ---------------------------------------------------------------------------
TRUST_BOUNDARY void send_to_untrusted(HandshakeState s);

// ---------------------------------------------------------------------------
// Trusted constructor.
//
// Misconfiguration 1 (DM-10): digest32 consumes (&s, sizeof(s)) — the full
// in-memory image of s including indeterminate padding bytes. The resulting
// tag is a function of padding, making it nondeterministic across runs and
// platforms, and potentially leaking stack residues into the tag value.
//
// Misconfiguration 2 (TB-DL2): the struct is returned by value and later
// passed by value to send_to_untrusted, transferring padding bytes across
// the trust boundary.
// ---------------------------------------------------------------------------
static HandshakeState build_state(uint64_t nonce, uint32_t msg_len) {
    HandshakeState s;       /* automatic storage: padding is indeterminate */
    s.version = 1;
    s.nonce   = nonce;
    s.msg_len = msg_len;

    /* DM-10: authenticating the raw in-memory image, not a canonical encoding.
     * The tag now depends on whatever happens to be in the padding bytes. */
    digest32(&s, sizeof(s), (uint8_t *)s.tag);

    return s;   /* return-by-value: padding transferred to caller */
}

int main(void) {
    HandshakeState st = build_state(0x1111222233334444ULL, 128);

    /* TB-DL2 + checker trigger: by-value transfer at annotated boundary.
     * security-misc-padding-boundary-leak should fire here — HandshakeState
     * has padding and no whole-object init is visible before this call. */
    send_to_untrusted(st);

    return 0;
}
