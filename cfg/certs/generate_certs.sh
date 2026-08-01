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

openssl req -x509 -newkey rsa:2048 -nodes \
    -keyout "${KEY}" -out "${CERT}" \
    -days 3650 \
    -subj "/CN=localhost" \
    -addext "subjectAltName=DNS:localhost,IP:127.0.0.1"

chmod 600 "${KEY}"

echo "Created self-signed test certificates:"
echo "  ${CERT}"
echo "  ${KEY}"
echo "Replace them with real certificates for production use."
