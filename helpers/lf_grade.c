#include "lf_grade.h"

#include <stdio.h>
#include <string.h>

/* ------------------------------------------------------------------ scale --
 *
 * Score = auth + integrity + obscurity + keyspace, out of 100.
 *
 *   auth       0..45   Does the credential prove it is itself? At 125 kHz the
 *                      answer is always no, so this term is always 0. It is
 *                      kept in the model - and in the report - because the
 *                      missing 45 points *are* the story.
 *   integrity  0..15   Can a corrupted or invented ID be spotted? Parity is
 *                      worth little, a real checksum a bit more. Capped low on
 *                      purpose: the attacker computes the checksum too.
 *   obscurity  0..25   Does the payload need a decoder that knows the format?
 *                      This is the only term that meaningfully separates the
 *                      formats, and it buys exactly one thing - it prices out
 *                      the attacker who owns a bargain cloner and nothing else.
 *   keyspace   0..15   Once an attacker has read one badge from the site, how
 *                      much is left to guess for the next one?
 *
 * The letter thresholds are Warden's, unchanged, so an F on a 125 kHz badge and
 * an F on a 13.56 MHz badge mean the same thing.                             */

#define BST_BAND_OBSCURED_MIN  28
#define BST_BAND_CLONEABLE_MIN 15

/* Findings carried per protocol on top of the ones the engine always emits. */
#define BST_PROTO_FINDINGS 4u

typedef struct {
    const char* name;
    const char* family;
    LfScoreParts parts;
    LfMod mod; /* what the format normally uses, if the read did not say */
    uint16_t id_bits;
    uint16_t guess_bits;
    const char* headline;
    const char* verdict;
    LfFindSeverity sev[BST_PROTO_FINDINGS];
    const char* find[BST_PROTO_FINDINGS];
} LfProtoInfo;

/* clang-format off */
static const LfProtoInfo lf_protos[LfProtoCount] = {
    [LfProtoEM4100] = {
        .name = "EM4100 / EM4102",
        .family = "EM Microelectronic",
        .parts = {.auth = 0, .integrity = 4, .obscurity = 0, .keyspace = 9},
        .mod = LfModASK, .id_bits = 40, .guess_bits = 32,
        .headline = "A 40-bit number, shouted in the clear",
        .verdict =
            "EM4100 is the barcode of access control. The tag holds one 40-bit "
            "number and reads it out, unchanged, to anything that energises it - "
            "no keys, no challenge, no session. Copying it is not an attack on "
            "EM4100; it is the tag working exactly as designed, for the wrong "
            "reader. Blank T5577 and EM4305 cards cost pennies and accept any ID "
            "you hand them, so the copy is indistinguishable at the door. Treat "
            "an EM4100 badge as a name tag: fine for telling people apart, "
            "useless for keeping people out.",
        .sev = {LfFindCritical, LfFindWarn, LfFindInfo, LfFindInfo},
        .find = {
            "40-bit ID and nothing else: no fields, no format",
            "Blank T5577/EM4305 clones cost under a dollar",
            "14 parity bits catch noise, never a forgery",
            "Sold as 'proximity'; it is proximity, not security",
        },
    },
    [LfProtoEM410032] = {
        .name = "EM4100 (32-bit)",
        .family = "EM Microelectronic",
        .parts = {.auth = 0, .integrity = 4, .obscurity = 0, .keyspace = 9},
        .mod = LfModASK, .id_bits = 32, .guess_bits = 32,
        .headline = "EM4100 with a narrower identifier",
        .verdict =
            "A 32-bit reading of the EM4100 frame - the same tag and the same "
            "exchange, with the identifier interpreted a byte shorter. Every "
            "EM4100 weakness applies without modification: a fixed number, sent "
            "in clear ASK to any reader that powers the tag, with parity as the "
            "only check and no secret anywhere in the protocol. The shorter "
            "field also means fewer distinct badges before a site starts reusing "
            "numbers.",
        .sev = {LfFindCritical, LfFindWarn, LfFindInfo, LfFindInfo},
        .find = {
            "32-bit variant: narrower than standard EM4100",
            "Blank T5577/EM4305 clones cost under a dollar",
            "Parity bits catch noise, never a forgery",
            "Same silicon, same exchange, same exposure",
        },
    },
    [LfProtoEM410016] = {
        .name = "EM4100 (16-bit)",
        .family = "EM Microelectronic",
        .parts = {.auth = 0, .integrity = 4, .obscurity = 0, .keyspace = 4},
        .mod = LfModASK, .id_bits = 16, .guess_bits = 16,
        .headline = "A 16-bit ID - the whole space is 65,536",
        .verdict =
            "The narrowest EM4100 reading, and the weakest credential Bastion "
            "can grade. Sixteen bits is 65,536 possible cards in total - not per "
            "site, in total - which is small enough to write out end to end. "
            "There is no authentication to fall back on and no secret to "
            "protect: the number is the credential, it is sent in the clear, and "
            "the space it lives in is tiny.",
        .sev = {LfFindCritical, LfFindCritical, LfFindWarn, LfFindInfo},
        .find = {
            "16-bit ID: the entire space is 65,536 numbers",
            "Short frame - enumerable from end to end",
            "Blank T5577/EM4305 clones cost under a dollar",
            "Parity only; no key material anywhere",
        },
    },
    [LfProtoElectra] = {
        .name = "Electra (EM4100)",
        .family = "EM Microelectronic",
        .parts = {.auth = 0, .integrity = 4, .obscurity = 1, .keyspace = 9},
        .mod = LfModASK, .id_bits = 40, .guess_bits = 32,
        .headline = "An EM4100 payload under a vendor profile",
        .verdict =
            "Electra credentials are EM4100 tags read under a vendor-specific "
            "profile. The silicon, the modulation and the exchange are all "
            "EM4100's, so the security properties are EM4100's too: a fixed "
            "40-bit number, broadcast in clear ASK, protected by parity and "
            "nothing else. The profile changes how the number is presented, not "
            "whether it can be copied.",
        .sev = {LfFindCritical, LfFindWarn, LfFindInfo, LfFindInfo},
        .find = {
            "EM4100 payload under a different reader profile",
            "Inherits every EM4100 weakness unchanged",
            "Parity only; no keys anywhere in the exchange",
            "Blank clones accept the ID without complaint",
        },
    },
    [LfProtoH10301] = {
        .name = "HID Prox H10301 26-bit",
        .family = "HID Prox",
        .parts = {.auth = 0, .integrity = 2, .obscurity = 3, .keyspace = 4},
        .mod = LfModFSK, .id_bits = 26, .guess_bits = 16,
        .headline = "26 bits, and only 16 of them are yours",
        .verdict =
            "The most deployed access credential in the world, and the weakest "
            "format in this list. H10301 packs an 8-bit facility code and a "
            "16-bit card number into 26 bits, with two parity bits and no "
            "cryptography of any kind. The facility code is identical on every "
            "badge the site ever issued, so reading one badge hands over the "
            "site half of all of them - leaving a 16-bit card number, a space "
            "small enough to write out in full. HID has sold the replacement "
            "since 2010. If your doors still take 26-bit prox, that is the "
            "finding.",
        .sev = {LfFindCritical, LfFindWarn, LfFindWarn, LfFindInfo},
        .find = {
            "16-bit card number: 65,536 values, total",
            "Two parity bits are the only integrity check",
            "Cloned to a T5577 blank in a single pass",
            "HID has shipped the replacement since 2010",
        },
    },
    [LfProtoIdteck] = {
        .name = "Idteck",
        .family = "Idteck",
        .parts = {.auth = 0, .integrity = 4, .obscurity = 3, .keyspace = 9},
        .mod = LfModASK, .id_bits = 64, .guess_bits = 32,
        .headline = "A wide frame, entirely in the clear",
        .verdict =
            "Idteck credentials carry a wide plaintext identifier over ASK. The "
            "extra width is worth something against an attacker guessing blind - "
            "there is more to guess - but nothing at all against one holding a "
            "reader, because the tag hands the whole thing over on request. "
            "There is no key, no challenge and no session; the frame that opens "
            "the door today is the same frame that opened it last year.",
        .sev = {LfFindCritical, LfFindWarn, LfFindGood, LfFindInfo},
        .find = {
            "Plaintext identifier, no key material at all",
            "Decoded by every mainstream RFID tool",
            "Wide frame: little to guess without a card",
            "ASK carrier - simple cloners reach it",
        },
    },
    [LfProtoIndala26] = {
        .name = "Indala 26-bit",
        .family = "Indala / Motorola",
        .parts = {.auth = 0, .integrity = 3, .obscurity = 11, .keyspace = 4},
        .mod = LfModPSK, .id_bits = 26, .guess_bits = 16,
        .headline = "PSK and a hidden layout - still a fixed number",
        .verdict =
            "Indala encodes 26 bits with phase-shift keying in a bit order the "
            "vendor never published. That combination did real work in the "
            "1990s: it kept the credential off the shelf of anyone without "
            "Indala's own reader. The encoding has since been reimplemented in "
            "every open RFID tool, so the obscurity now prices out only the "
            "casual attacker with an ASK-only cloner. Underneath it is the same "
            "static, unauthenticated number as everything else at this "
            "frequency.",
        .sev = {LfFindWarn, LfFindWarn, LfFindInfo, LfFindInfo},
        .find = {
            "Undocumented bit order: obscurity, not security",
            "Public tools decode and replay it directly",
            "26 bits - the same width as HID prox",
            "PSK carrier defeats ASK-only cloners",
        },
    },
    [LfProtoIOProxXSF] = {
        .name = "ioProx XSF",
        .family = "Kantech",
        .parts = {.auth = 0, .integrity = 7, .obscurity = 4, .keyspace = 4},
        .mod = LfModFSK, .id_bits = 32, .guess_bits = 16,
        .headline = "Kantech's FSK format, plainly readable",
        .verdict =
            "ioProx XSF carries a version, an 8-bit facility code and a 16-bit "
            "card number over FSK, with a checksum that guards against a misread "
            "rather than a forgery. No key material appears anywhere in the "
            "exchange. As with every Wiegand-derived format, the facility code "
            "is site-wide, so the only unknown left is a 16-bit card number - and "
            "reading any single badge on the site supplies the rest.",
        .sev = {LfFindCritical, LfFindWarn, LfFindInfo, LfFindInfo},
        .find = {
            "Site-wide facility code + 16-bit card number",
            "Checksum detects a misread, not a forged card",
            "Format is fully documented in open tooling",
            "FSK carrier - needs a demodulating cloner",
        },
    },
    [LfProtoAwid] = {
        .name = "AWID 26-bit",
        .family = "AWID",
        .parts = {.auth = 0, .integrity = 4, .obscurity = 4, .keyspace = 4},
        .mod = LfModFSK, .id_bits = 26, .guess_bits = 16,
        .headline = "26-bit Wiegand over FSK, no keys",
        .verdict =
            "AWID's 26-bit format is HID's 26-bit format wearing different "
            "clothes: eight bits of facility, sixteen bits of card, parity, FSK "
            "on the wire, and nothing that resembles a secret. Any tool that "
            "demodulates FSK reads it and any T5577 blank replays it. The card "
            "cannot distinguish a genuine reader from a hostile one, because it "
            "never asks - it simply recites its number to whatever powers it up.",
        .sev = {LfFindCritical, LfFindWarn, LfFindInfo, LfFindInfo},
        .find = {
            "26 plaintext bits: facility code + card number",
            "Copied to a T5577 blank in a single pass",
            "HID's 26-bit weakness in different clothes",
            "Parity is for error detection only",
        },
    },
    [LfProtoFDXA] = {
        .name = "FDX-A animal tag",
        .family = "FECAVA / animal ID",
        .parts = {0, 0, 0, 0},
        .mod = LfModASK, .id_bits = 64, .guess_bits = 0,
        .headline = "A pet transponder, not a door key",
        .verdict =
            "This is an animal identification transponder - the chip a vet "
            "implants in a dog or cat - not an access credential. It holds a "
            "registry number and reads it out to any scanner, which is precisely "
            "what it was designed to do. Grading it against door-security "
            "criteria would be meaningless, so Bastion does not. The one thing "
            "worth saying: if this tag opens a door, someone has repurposed a pet "
            "chip as a key, and it carries no protection whatsoever.",
        .sev = {LfFindInfo, LfFindWarn, LfFindInfo, LfFindInfo},
        .find = {
            "Animal transponder (FECAVA), not a credential",
            "If this opens a door, a pet chip is the key",
            "No security design at all - it is a registry number",
            "Reads out to any scanner, by design",
        },
    },
    [LfProtoFDXB] = {
        .name = "FDX-B animal tag",
        .family = "ISO 11784/11785",
        .parts = {0, 0, 0, 0},
        .mod = LfModFSK, .id_bits = 128, .guess_bits = 0,
        .headline = "An ISO 11784 livestock/pet chip",
        .verdict =
            "An ISO 11784/11785 animal identification chip: a country code, a "
            "national registry number and a CRC-16, used for pets and livestock. "
            "It is not an access credential and has no security model to grade - "
            "it is a licence plate, and it is supposed to be readable by anyone "
            "with a scanner. The CRC protects the number against misreads, not "
            "against copying. If this tag is being used to open a door, that is "
            "the finding.",
        .sev = {LfFindInfo, LfFindWarn, LfFindGood, LfFindInfo},
        .find = {
            "ISO 11784/11785 livestock & pet ID chip",
            "If this opens a door, a pet chip is the key",
            "CRC-16 protects the number against misreads",
            "Country code + national registry number",
        },
    },
    [LfProtoHidGeneric] = {
        .name = "HID Prox (generic)",
        .family = "HID Prox",
        .parts = {.auth = 0, .integrity = 3, .obscurity = 3, .keyspace = 7},
        .mod = LfModFSK, .id_bits = 0, .guess_bits = 24,
        .headline = "A Wiegand payload in the clear",
        .verdict =
            "An HID proximity credential in a format wider than the classic "
            "26-bit card. The extra bits usually buy a larger card number and "
            "sometimes an issue code, which does help against blind guessing - "
            "but the payload is still plaintext Wiegand with no cryptography, and "
            "the site code is still shared across every badge in the building. "
            "Width raises the cost of guessing a card you have never seen. It "
            "does nothing about the card in someone's pocket.",
        .sev = {LfFindCritical, LfFindWarn, LfFindGood, LfFindInfo},
        .find = {
            "Wiegand payload in the clear, whatever its width",
            "Site code is still shared across all badges",
            "Wider than 26-bit: more to guess without a card",
            "No cryptography anywhere in the exchange",
        },
    },
    [LfProtoHidExGeneric] = {
        .name = "HID Prox (extended)",
        .family = "HID Prox",
        .parts = {.auth = 0, .integrity = 3, .obscurity = 3, .keyspace = 8},
        .mod = LfModFSK, .id_bits = 0, .guess_bits = 28,
        .headline = "Extended-width Wiegand, still plaintext",
        .verdict =
            "An extended-length HID proximity format. The long frame carries a "
            "larger card number and often an issue level, so the space an "
            "attacker must search without a sample card is genuinely bigger than "
            "a 26-bit card's. That is the whole of the improvement. The frame is "
            "not encrypted, the reader issues no challenge, and a single "
            "recording still reproduces the credential exactly.",
        .sev = {LfFindCritical, LfFindGood, LfFindWarn, LfFindInfo},
        .find = {
            "Extended Wiegand, still entirely plaintext",
            "Wide frame: more to guess without a sample card",
            "Site code remains shared across badges",
            "One recording still reproduces the credential",
        },
    },
    [LfProtoPyramid] = {
        .name = "Farpointe Pyramid",
        .family = "Farpointe Data",
        .parts = {.auth = 0, .integrity = 5, .obscurity = 4, .keyspace = 4},
        .mod = LfModFSK, .id_bits = 26, .guess_bits = 16,
        .headline = "Vendor framing over an ordinary Wiegand payload",
        .verdict =
            "Pyramid wraps a standard Wiegand payload - typically eight bits of "
            "facility and sixteen of card number - in Farpointe's own FSK framing "
            "with a checksum. The framing is proprietary; the payload is not, and "
            "neither is protected by a key. Once a tool knows the framing, and "
            "the common ones all do, the credential is a fixed number again, "
            "recorded and replayed like any other.",
        .sev = {LfFindCritical, LfFindWarn, LfFindWarn, LfFindInfo},
        .find = {
            "Ordinary Wiegand payload under vendor framing",
            "Proprietary framing only slows the unprepared",
            "Checksum guards transmission, not authenticity",
            "Facility code shared across the site",
        },
    },
    [LfProtoViking] = {
        .name = "Viking",
        .family = "Viking",
        .parts = {.auth = 0, .integrity = 7, .obscurity = 3, .keyspace = 9},
        .mod = LfModASK, .id_bits = 64, .guess_bits = 32,
        .headline = "64 bits with a checksum - still a fixed ID",
        .verdict =
            "Viking sends a 64-bit frame with a checksum over ASK. The larger "
            "frame means less of it is guessable than a 26-bit Wiegand card, and "
            "the checksum makes a random guess unlikely to be accepted. Neither "
            "addresses the actual problem, which is that the tag has no way to "
            "prove it is the tag. A frame recorded once replays perfectly, "
            "forever, to every reader on the system.",
        .sev = {LfFindCritical, LfFindWarn, LfFindGood, LfFindInfo},
        .find = {
            "Static frame - replays perfectly once recorded",
            "The checksum is not a signature",
            "64-bit frame: little to guess without a card",
            "ASK carrier - within reach of simple cloners",
        },
    },
    [LfProtoJablotron] = {
        .name = "Jablotron",
        .family = "Jablotron",
        .parts = {.auth = 0, .integrity = 7, .obscurity = 4, .keyspace = 9},
        .mod = LfModASK, .id_bits = 40, .guess_bits = 32,
        .headline = "40 bits plus a checksum, sent in clear",
        .verdict =
            "Jablotron's ASK format carries a 40-bit identifier with a checksum. "
            "It is well-behaved and stable, and it is entirely unauthenticated: "
            "the number on the card is the whole credential. It turns up most "
            "often on European alarm and access panels, where it is frequently "
            "paired with a keypad - and where that is the case, the code being "
            "typed is doing all of the security work.",
        .sev = {LfFindCritical, LfFindWarn, LfFindGood, LfFindInfo},
        .find = {
            "Unauthenticated 40-bit ID, clear on the wire",
            "Checksum proves integrity, never origin",
            "40-bit space resists blind guessing",
            "Often paired with a PIN - the PIN is the real key",
        },
    },
    [LfProtoParadox] = {
        .name = "Paradox",
        .family = "Paradox",
        .parts = {.auth = 0, .integrity = 6, .obscurity = 7, .keyspace = 7},
        .mod = LfModFSK, .id_bits = 40, .guess_bits = 24,
        .headline = "Vendor framing over a plaintext ID",
        .verdict =
            "Paradox credentials carry a manufacturer code alongside the facility "
            "and card numbers, framed over FSK in a layout the vendor does not "
            "publish. That raises the bar past a generic cloner and no further: "
            "the payload is not encrypted, the reader issues no challenge, and "
            "the frame is stable enough to record and replay verbatim. The "
            "manufacturer code narrows an attacker's search. It does not close "
            "it.",
        .sev = {LfFindCritical, LfFindWarn, LfFindWarn, LfFindInfo},
        .find = {
            "Vendor framing over a static, plaintext payload",
            "Manufacturer code narrows the search, never blocks it",
            "Frames replay verbatim once recorded",
            "Layout is public in open tooling",
        },
    },
    [LfProtoPACStanley] = {
        .name = "PAC / Stanley",
        .family = "PAC (Stanley)",
        .parts = {.auth = 0, .integrity = 5, .obscurity = 4, .keyspace = 7},
        .mod = LfModASK, .id_bits = 32, .guess_bits = 24,
        .headline = "A plain 32-bit credential",
        .verdict =
            "PAC (Stanley) sends a 32-bit identifier over ASK with a checksum. It "
            "is widely installed across UK sites and completely unauthenticated - "
            "the card recites a number and the door believes it. The format is "
            "fully supported by open tooling, so there is not even framing "
            "obscurity left to lean on: read it once and it is reproduced.",
        .sev = {LfFindCritical, LfFindWarn, LfFindInfo, LfFindInfo},
        .find = {
            "32-bit plaintext ID with no protection",
            "Fully supported by open-source tools",
            "Checksum covers misreads only",
            "ASK carrier - cheap cloners handle it",
        },
    },
    [LfProtoKeri] = {
        .name = "Keri",
        .family = "Keri Systems",
        .parts = {.auth = 0, .integrity = 4, .obscurity = 13, .keyspace = 6},
        .mod = LfModPSK, .id_bits = 32, .guess_bits = 20,
        .headline = "Indala-derived PSK with a light scramble",
        .verdict =
            "Keri Systems credentials ride Indala's PSK carrier with their own "
            "arrangement of a facility and card number. The scramble is light and "
            "was reversed in public tooling long ago, so what remains is a static "
            "identifier that any PSK-capable reader can lift and any T5577 blank "
            "can carry. The PSK carrier is the real barrier here, and it only "
            "stops attackers whose hardware cannot demodulate phase.",
        .sev = {LfFindWarn, LfFindWarn, LfFindInfo, LfFindInfo},
        .find = {
            "Bit scramble reversed long ago in public tools",
            "PSK carrier keeps out only ASK-only cloners",
            "Facility + card number under the scramble",
            "Built on the Indala format family",
        },
    },
    [LfProtoGallagher] = {
        .name = "Gallagher / Cardax",
        .family = "Gallagher",
        .parts = {.auth = 0, .integrity = 10, .obscurity = 20, .keyspace = 8},
        .mod = LfModASK, .id_bits = 96, .guess_bits = 24,
        .headline = "The best-built LF format here - still a replay",
        .verdict =
            "Cardax is the best-engineered credential at 125 kHz. Region, "
            "facility, card number and issue level are run through a proprietary "
            "obfuscation, so the bits on the wire are not the bits on the badge, "
            "and the issue level lets an operator invalidate a lost card without "
            "reissuing its number. That is more than any other format here "
            "offers. It is still not authentication: the obfuscation is a fixed "
            "transform with no secret held by the reader, it has been "
            "reimplemented in open tools, and the resulting frame never changes - "
            "so a recording is a working copy. Gallagher's own answer is their "
            "13.56 MHz credential.",
        .sev = {LfFindGood, LfFindGood, LfFindWarn, LfFindWarn},
        .find = {
            "Obfuscated payload: wire bits are not badge bits",
            "Issue level can invalidate a lost card",
            "Fixed transform, no secret: a recording still works",
            "Vendor ships a 13.56 MHz replacement",
        },
    },
    [LfProtoNexwatch] = {
        .name = "Nexwatch / Nedap",
        .family = "Nedap",
        .parts = {.auth = 0, .integrity = 9, .obscurity = 18, .keyspace = 8},
        .mod = LfModPSK, .id_bits = 128, .guess_bits = 24,
        .headline = "A scrambled payload over PSK",
        .verdict =
            "Nexwatch (Nedap-derived) scrambles its payload and protects it with "
            "a checksum, carried over PSK. Between the phase modulation and the "
            "scramble, a credential cannot be lifted by a generic cloner - which "
            "is a real difference from an EM4100 badge, and worth something "
            "against opportunistic attackers. Against a prepared one it is worth "
            "nothing: the scramble is a fixed transform that has been "
            "reimplemented publicly, and the descrambled credential is the same "
            "unchanging number as every other tag at this frequency.",
        .sev = {LfFindGood, LfFindWarn, LfFindWarn, LfFindInfo},
        .find = {
            "Scrambled payload with a checksum",
            "Scramble is a fixed transform, reimplemented publicly",
            "Descrambled credential never changes",
            "PSK carrier defeats ASK-only cloners",
        },
    },
    [LfProtoSecurakey] = {
        .name = "Securakey",
        .family = "Securakey",
        .parts = {.auth = 0, .integrity = 5, .obscurity = 4, .keyspace = 4},
        .mod = LfModASK, .id_bits = 26, .guess_bits = 16,
        .headline = "Facility and card number, in the open",
        .verdict =
            "Securakey credentials carry a facility code and a card number in a "
            "short ASK frame with no protection beyond a check field. The frame "
            "is narrow, so the space an attacker has to search is narrow with it, "
            "and open tooling decodes the format directly. Nothing in the "
            "exchange establishes that the card is genuine; the reader accepts "
            "the number because it arrived, not because it was proven.",
        .sev = {LfFindCritical, LfFindWarn, LfFindWarn, LfFindInfo},
        .find = {
            "Plain facility + card number, no protection",
            "Short frame keeps the guessing space small",
            "Fully decoded by mainstream tooling",
            "ASK carrier - cheap cloners handle it",
        },
    },
    [LfProtoGProxII] = {
        .name = "G-Prox-II",
        .family = "Guardall / Verex",
        .parts = {.auth = 0, .integrity = 9, .obscurity = 12, .keyspace = 7},
        .mod = LfModFSK, .id_bits = 36, .guess_bits = 20,
        .headline = "An obfuscated frame with a length tag",
        .verdict =
            "G-Prox-II carries an obfuscated payload in a length-tagged frame "
            "over FSK, which makes it awkward for a generic cloner and easy for a "
            "tool that knows the format. The obfuscation is a published, fixed "
            "transform - there is no per-card or per-reader secret involved - so "
            "it changes who can copy the badge, not whether the badge can be "
            "copied. The credential itself is the same static number the rest of "
            "125 kHz relies on.",
        .sev = {LfFindGood, LfFindWarn, LfFindWarn, LfFindInfo},
        .find = {
            "Obfuscated payload in a length-tagged frame",
            "Transform is public - decoders are in open tools",
            "No per-card or per-reader secret involved",
            "Guardall/Verex family; FSK carrier",
        },
    },
    [LfProtoNoralsy] = {
        .name = "Noralsy",
        .family = "Noralsy",
        .parts = {.auth = 0, .integrity = 6, .obscurity = 5, .keyspace = 7},
        .mod = LfModASK, .id_bits = 40, .guess_bits = 24,
        .headline = "A year field and a card number, in clear",
        .verdict =
            "Noralsy credentials carry a card number and a year field over ASK "
            "with a check value. Common on French residential and commercial "
            "door entry systems. The year field narrows the search for anyone "
            "guessing blind, which is a small real benefit, but the payload is "
            "plaintext and unauthenticated - the reader has no way to tell an "
            "original from a blank programmed five minutes ago.",
        .sev = {LfFindCritical, LfFindWarn, LfFindInfo, LfFindInfo},
        .find = {
            "Plaintext year + card number",
            "Check value is not authentication",
            "Reader cannot tell an original from a blank",
            "ASK carrier - within reach of simple cloners",
        },
    },
    [LfProtoUnknown] = {
        .name = "Unknown 125 kHz tag",
        .family = "Unrecognised",
        .parts = {0, 0, 0, 0},
        .mod = LfModUnknown, .id_bits = 0, .guess_bits = 0,
        .headline = "Nothing decoded",
        .verdict =
            "Bastion sensed no credential it could decode. That is usually "
            "placement: 125 kHz coupling is tight, so the badge needs to sit flat "
            "against the back of the Flipper, centred, with no metal and no phone "
            "in between. If it still will not read, the tag may use a format "
            "outside the firmware's decoder set - or it may not be a 125 kHz tag "
            "at all. A card that reads on a 13.56 MHz reader belongs in Warden "
            "instead.",
        .sev = {LfFindInfo, LfFindInfo, LfFindInfo, LfFindInfo},
        .find = {
            "Hold the badge flat against the Flipper's back",
            "Remove metal, phones and other cards",
            "Try forcing ASK or PSK in Settings",
            "13.56 MHz cards will never read here",
        },
    },
};
/* clang-format on */

/* ------------------------------------------------------------- labels ----- */

static const char* const band_labels[LfBandCount] = {
    "BROADCAST",
    "CLONEABLE",
    "OBSCURED",
    "NOT A KEY",
    "UNREAD",
};

static const char* const band_blurbs[LfBandCount] = {
    "A plaintext fixed ID. The badge shouts a number and anything listening can "
    "repeat it.",
    "Structured, but still plaintext. A decoder turns it straight back into a "
    "working copy.",
    "A proprietary encoding. It prices out a casual attacker and does not slow "
    "a prepared one.",
    "An animal transponder. It was never designed to secure anything.",
    "Nothing decoded - reposition the badge, or force a modulation in Settings.",
};

const char* lf_band_label(LfBand band) {
    if((unsigned)band >= LfBandCount) return band_labels[LfBandUnread];
    return band_labels[band];
}

const char* lf_band_blurb(LfBand band) {
    if((unsigned)band >= LfBandCount) return band_blurbs[LfBandUnread];
    return band_blurbs[band];
}

const char* lf_clone_time(LfCloneClass c) {
    switch(c) {
    case LfCloneInstant:
        return "~2 s";
    case LfCloneQuick:
        return "~5 s";
    case LfCloneTooled:
        return "~30 s";
    case LfCloneNotAKey:
        return "n/a";
    default:
        return "?";
    }
}

const char* lf_clone_label(LfCloneClass c) {
    switch(c) {
    case LfCloneInstant:
        return "Any handheld cloner";
    case LfCloneQuick:
        return "Any FSK/PSK reader";
    case LfCloneTooled:
        return "A tool that knows the format";
    case LfCloneNotAKey:
        return "Not an access credential";
    default:
        return "Unknown";
    }
}

/* The verdict screen gives this column 74 px - about fourteen characters in
 * FontSecondary - so it gets its own wording rather than a truncated sentence. */
const char* lf_clone_short(LfCloneClass c) {
    switch(c) {
    case LfCloneInstant:
        return "Cheap cloner";
    case LfCloneQuick:
        return "FSK/PSK tool";
    case LfCloneTooled:
        return "Format decoder";
    case LfCloneNotAKey:
        return "Not a key";
    default:
        return "Unknown";
    }
}

const char* lf_mod_label(LfMod mod) {
    switch(mod) {
    case LfModASK:
        return "ASK";
    case LfModFSK:
        return "FSK";
    case LfModPSK:
        return "PSK";
    default:
        return "?";
    }
}

const char* lf_severity_glyph(LfFindSeverity sev) {
    switch(sev) {
    case LfFindCritical:
        return "[x]";
    case LfFindWarn:
        return "[!]";
    case LfFindGood:
        return "[+]";
    default:
        return "[i]";
    }
}

const char* lf_proto_name(LfProto proto) {
    if((unsigned)proto >= LfProtoCount) return lf_protos[LfProtoUnknown].name;
    return lf_protos[proto].name;
}

const char* lf_score_letter(int score) {
    /* Warden's thresholds, so a grade means the same thing on both radios. */
    if(score >= 90) return "A+";
    if(score >= 80) return "A";
    if(score >= 65) return "B";
    if(score >= 50) return "C";
    if(score >= 35) return "D";
    return "F";
}

/* ------------------------------------------------------------ evaluate ---- */

static void add_finding(LfGrade* g, LfFindSeverity sev, const char* text) {
    if(g->finding_num >= BST_MAX_FINDINGS) return;
    LfFinding* f = &g->findings[g->finding_num++];
    f->sev = sev;
    /* snprintf, not strncpy: guarantees the terminator even on a long source. */
    snprintf(f->text, sizeof(f->text), "%s", text);
}

static void add_findingf(LfGrade* g, LfFindSeverity sev, const char* fmt, unsigned a, unsigned b) {
    if(g->finding_num >= BST_MAX_FINDINGS) return;
    LfFinding* f = &g->findings[g->finding_num++];
    f->sev = sev;
    snprintf(f->text, sizeof(f->text), fmt, a, b);
}

static LfBand band_from_score(int score) {
    if(score >= BST_BAND_OBSCURED_MIN) return LfBandObscured;
    if(score >= BST_BAND_CLONEABLE_MIN) return LfBandCloneable;
    return LfBandBroadcast;
}

static LfCloneClass clone_from(const LfScoreParts* p, LfMod mod) {
    /* What an attacker needs to own. Obfuscation means a format-aware tool;
     * otherwise phase/frequency modulation is the only thing standing between
     * the badge and the cheapest cloner on the market. */
    if(p->obscurity >= 12) return LfCloneTooled;
    if(mod == LfModFSK || mod == LfModPSK) return LfCloneQuick;
    return LfCloneInstant;
}

/* Render `len` bytes as spaced hex. If they do not all fit, render as many as
 * do and mark the cut with " .." rather than stopping mid-number - a silently
 * short ID would read as a different card. */
static void hex_line(char* out, size_t out_sz, const uint8_t* data, uint8_t len) {
    if(out_sz == 0) return;
    out[0] = '\0';
    if(!data || len == 0) return;

    /* n bytes cost 2 + 3*(n-1) chars, so `room` chars hold 1 + (room-2)/3. */
    const size_t room = out_sz - 1;
    size_t fits = (room >= 2) ? (1 + (room - 2) / 3) : 0;
    bool clipped = false;
    if(len > fits) {
        fits = (room >= 5) ? (1 + (room - 5) / 3) : 0; /* reserve " .." */
        clipped = true;
    } else {
        fits = len; /* everything fits - render exactly what we were given */
    }

    size_t used = 0;
    for(size_t i = 0; i < fits; i++) {
        int n = snprintf(out + used, out_sz - used, i ? " %02X" : "%02X", data[i]);
        if(n <= 0) return;
        used += (size_t)n;
    }
    if(clipped && used + 3 < out_sz) {
        out[used++] = ' ';
        out[used++] = '.';
        out[used++] = '.';
        out[used] = '\0';
    }
}

void lf_grade_evaluate(const LfReading* reading, LfGrade* out) {
    if(!out) return;
    memset(out, 0, sizeof(*out));

    LfProto proto = LfProtoUnknown;
    LfMod mod = LfModUnknown;
    uint8_t data_len = 0;
    const uint8_t* data = NULL;

    if(reading && (unsigned)reading->proto < LfProtoCount) {
        proto = reading->proto;
        mod = reading->mod;
        data_len = reading->data_len > BST_MAX_DATA ? BST_MAX_DATA : reading->data_len;
        data = reading->data;
    }

    const LfProtoInfo* info = &lf_protos[proto];
    if(mod == LfModUnknown) mod = info->mod; /* fall back to the format's usual */

    snprintf(out->name, sizeof(out->name), "%s", info->name);
    snprintf(out->family, sizeof(out->family), "%s", info->family);
    snprintf(out->headline, sizeof(out->headline), "%s", info->headline);
    snprintf(out->verdict, sizeof(out->verdict), "%s", info->verdict);
    out->parts = info->parts;
    out->id_bits = info->id_bits;
    out->guess_bits = info->guess_bits;

    const bool is_animal = (proto == LfProtoFDXA || proto == LfProtoFDXB);
    const bool is_unread = (proto == LfProtoUnknown);

    if(is_unread) {
        out->scored = false;
        out->score = 0;
        out->band = LfBandUnread;
        out->clone = LfCloneUnknown;
        snprintf(out->letter, sizeof(out->letter), "-");
        snprintf(out->id_line, sizeof(out->id_line), "no decode");
    } else if(is_animal) {
        /* A number here would be a lie: this was never a security product. */
        out->scored = false;
        out->score = 0;
        out->band = LfBandNotAKey;
        out->clone = LfCloneNotAKey;
        snprintf(out->letter, sizeof(out->letter), "-");
    } else {
        out->scored = true;
        int score = (int)info->parts.auth + (int)info->parts.integrity +
                    (int)info->parts.obscurity + (int)info->parts.keyspace;
        if(score < 0) score = 0;
        if(score > 100) score = 100;
        out->score = score;
        out->band = band_from_score(score);
        out->clone = clone_from(&info->parts, mod);
        snprintf(out->letter, sizeof(out->letter), "%s", lf_score_letter(score));
    }

    /* ------------------------------------------------------- the ID line -- */
    if(!is_unread) {
        if(proto == LfProtoH10301 && data_len >= 3) {
            unsigned fc = data[0];
            unsigned card = ((unsigned)data[1] << 8) | data[2];
            snprintf(out->id_line, sizeof(out->id_line), "FC %u  Card %u", fc, card);
        } else if(
            (proto == LfProtoEM4100 || proto == LfProtoEM410032 || proto == LfProtoElectra) &&
            data_len >= 5) {
            unsigned cust = data[0];
            unsigned long sn = ((unsigned long)data[1] << 24) | ((unsigned long)data[2] << 16) |
                               ((unsigned long)data[3] << 8) | data[4];
            snprintf(out->id_line, sizeof(out->id_line), "Cust %02X  SN %lu", cust, sn);
        } else if(data_len > 0) {
            hex_line(out->id_line, sizeof(out->id_line), data, data_len);
        } else {
            snprintf(out->id_line, sizeof(out->id_line), "-");
        }
    }

    /* -------------------------------------------------------- findings ---- *
     * Engine-generated truths first (they are the same for every access
     * credential at this frequency and they are the most important thing on the
     * screen), then anything decoded from this specific badge, then the
     * format's own notes filling whatever room is left.                      */
    if(!is_unread && !is_animal) {
        add_finding(out, LfFindCritical, "No authentication: any reader gets the same answer");

        if(mod == LfModASK) {
            add_finding(out, LfFindCritical, "Plain ASK carrier - the cheapest cloners read it");
        } else {
            add_finding(out, LfFindWarn, "FSK/PSK stops bargain cloners, not a Flipper");
        }

        if(info->guess_bits > 0 && info->guess_bits <= 20) {
            add_findingf(
                out,
                LfFindCritical,
                "One read leaves %u bits (%u values) to guess",
                info->guess_bits,
                (unsigned)1u << (info->guess_bits > 20 ? 20 : info->guess_bits));
        }

        if(proto == LfProtoH10301 && data_len >= 3) {
            add_findingf(
                out,
                LfFindCritical,
                "Facility %u is on every badge at this site (card %u)",
                data[0],
                ((unsigned)data[1] << 8) | data[2]);
        }
    }

    for(unsigned i = 0; i < BST_PROTO_FINDINGS; i++) {
        if(!info->find[i]) break;
        add_finding(out, info->sev[i], info->find[i]);
    }
}
