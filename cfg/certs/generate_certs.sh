#!/bin/sh
# Generate a self-signed TLS certificate/key pair for local development.
#
# Run once during initial setup:
#   ./cfg/certs/generate_certs.sh
#
# The certificates are referenced from cfg/bootstrap.xml via the relative
# paths certs/server.crt and certs/server.key.
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
CERT="${SCRIPT_DIR}/server.crt"
KEY="${SCRIPT_DIR}/server.key"

if [ -f "${CERT}" ] && [ -f "${KEY}" ]; then
    echo "Certificates already exist:"
    echo "  ${CERT}"
    echo "  ${KEY}"
    echo "Delete them first if you want to regenerate."
    exit 0
fi

# The subject alternative name is written through a config file instead of
# `openssl req -addext`: -addext needs OpenSSL 1.1.1+, which the macOS system
# `openssl` (LibreSSL) does not provide. A config file with `x509_extensions`
# works on both OpenSSL and LibreSSL.
CONFIG="$(mktemp)"
trap 'rm -f "${CONFIG}"' EXIT
cat > "${CONFIG}" <<'EOF'
[req]
distinguished_name = req_distinguished_name
x509_extensions = v3_req
prompt = no

[req_distinguished_name]
CN = localhost

[v3_req]
subjectAltName = DNS:localhost,IP:127.0.0.1
keyUsage = critical, digitalSignature, keyEncipherment
extendedKeyUsage = serverAuth
EOF

openssl req -x509 -newkey rsa:2048 -nodes \
    -keyout "${KEY}" -out "${CERT}" \
    -days 3650 \
    -config "${CONFIG}"

chmod 600 "${KEY}"

echo "Created self-signed test certificates:"
echo "  ${CERT}"
echo "  ${KEY}"
echo "Replace them with real certificates for production use."
