"""Client-side crypto used by the "Yoga" framework in My Sweet Roomies!.

Recovered from jp.mfapps.framework.maidengine.util.Security:

    key = MD5( Signature.toCharsString() )   # toCharsString() == lowercase hex of the DER signing cert
    iv  = b"0123456789abcdef"
    AES/CBC/PKCS5Padding, output is Base64 with flags 27 = URL_SAFE | NO_WRAP | NO_PADDING | NO_CLOSE

It is used for:
  * the X-Yoga-Authorization header  -> JSON {"device_id", "timestamp", "uuid"}
  * the purchase_data POST field     -> Google Play purchase "originalJson"

Because the key is derived from the APK *signing certificate*, a re-signed
(patched) APK produces a different key. That's why the key can be derived from
whichever APK you point the server at.
"""

import base64
import hashlib
import zipfile

from cryptography.hazmat.primitives import padding
from cryptography.hazmat.primitives.ciphers import Cipher, algorithms, modes
from cryptography.hazmat.primitives.serialization import Encoding, pkcs7

IV = b"0123456789abcdef"

# MD5 of the original Fuji Television / BitStudio signing cert shipped in v1.4.6.
ORIGINAL_KEY_HEX = "e9ee6770731a5c5b7dff9b7f827cadb8"


def key_from_apk(apk_path):
    """Derive the AES key the way Security.getApplicationSignature() does on-device."""
    with zipfile.ZipFile(apk_path) as apk:
        sig_name = next(
            n for n in apk.namelist()
            if n.startswith("META-INF/") and n.upper().endswith((".RSA", ".DSA", ".EC"))
        )
        certs = pkcs7.load_der_pkcs7_certificates(apk.read(sig_name))
    der = certs[0].public_bytes(Encoding.DER)
    return hashlib.md5(der.hex().encode("ascii")).digest()


def _b64decode(data):
    data = data.strip()
    return base64.urlsafe_b64decode(data + "=" * (-len(data) % 4))


def decrypt(key, token):
    cipher = Cipher(algorithms.AES(key), modes.CBC(IV)).decryptor()
    raw = cipher.update(_b64decode(token)) + cipher.finalize()
    unpadder = padding.PKCS7(128).unpadder()
    return (unpadder.update(raw) + unpadder.finalize()).decode("utf-8")


def encrypt(key, text):
    """Mirror of the client, handy for tests and for crafting requests by hand."""
    padder = padding.PKCS7(128).padder()
    data = padder.update(text.encode("utf-8")) + padder.finalize()
    cipher = Cipher(algorithms.AES(key), modes.CBC(IV)).encryptor()
    out = cipher.update(data) + cipher.finalize()
    return base64.urlsafe_b64encode(out).decode("ascii").rstrip("=")
