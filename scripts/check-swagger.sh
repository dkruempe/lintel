#!/usr/bin/env bash
#
# check-swagger.sh -- lint the API specification and keep the markdown mirror in
# sync with it.
#
# `swagger.yaml` (Swagger 2.0) is the wire-contract source of truth; `swagger.md`
# is a human-readable mirror. The top-level CMakeLists.txt already fails the
# configure step when `info.version` differs from LINTEL_VERSION, but that misses
# two classes of drift:
#   * a `$ref` that points at a definition/parameter/response that does not exist
#     (renders as a broken endpoint, surprise at runtime),
#   * an operation without a `responses` block, and
#   * a stale `## Version:` line in `swagger.md`.
# This script checks all of them. It uses PyYAML; if the optional
# swagger-spec-validator is installed it additionally runs the full Swagger 2.0
# schema validation.
#
# Usage: ./scripts/check-swagger.sh [swagger.yaml] [swagger.md]

set -euo pipefail

PROJ_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SPEC="${1:-$PROJ_ROOT/swagger.yaml}"
MIRROR="${2:-$PROJ_ROOT/swagger.md}"
CMAKE_FILE="$PROJ_ROOT/CMakeLists.txt"

python3 - "$SPEC" "$MIRROR" "$CMAKE_FILE" <<'PY'
import re
import sys

try:
    import yaml
except ImportError:
    sys.exit("PyYAML is required (install python3-yaml)")

spec_path, mirror_path, cmake_path = sys.argv[1:4]

with open(cmake_path, encoding='utf-8') as handle:
    cmake = handle.read()
match = re.search(r'set\(LINTEL_VERSION\s+"([^"]+)"', cmake)
if not match:
    sys.exit("could not read LINTEL_VERSION from CMakeLists.txt")
project_version = match.group(1)

with open(spec_path, encoding='utf-8') as handle:
    try:
        spec = yaml.safe_load(handle)
    except yaml.YAMLError as error:
        sys.exit(f"swagger.yaml is not valid YAML: {error}")

problems = []


def problem(message):
    problems.append(message)


if not isinstance(spec, dict):
    sys.exit("swagger.yaml does not contain a mapping at the top level")

if str(spec.get('swagger', '')) != '2.0':
    problem(f"top-level 'swagger' must be \"2.0\", found {spec.get('swagger')!r}")

info = spec.get('info') or {}
if not info.get('title'):
    problem("info.title is missing")
spec_version = info.get('version')
if not spec_version:
    problem("info.version is missing")
elif spec_version != project_version:
    problem(f"info.version {spec_version!r} != LINTEL_VERSION {project_version!r}")

# --- collect every $ref target that exists -----------------------------------
definitions = set((spec.get('definitions') or {}).keys())
parameters = set((spec.get('parameters') or {}).keys())
responses = set((spec.get('responses') or {}).keys())
security_definitions = set((spec.get('securityDefinitions') or {}).keys())
ref_sections = {
    'definitions': definitions,
    'parameters': parameters,
    'responses': responses,
    'securityDefinitions': security_definitions,
}

# --- walk the document: check refs, operations and responses -----------------
HTTP_METHODS = {'get', 'put', 'post', 'delete', 'options', 'head', 'patch'}

def walk(node, path):
    if isinstance(node, dict):
        ref = node.get('$ref')
        if isinstance(ref, str) and ref.startswith('#/'):
            parts = ref[2:].split('/')
            section = parts[0]
            name = parts[-1]
            if section in ref_sections and name not in ref_sections[section]:
                problem(f"dangling $ref '{ref}' at {path}")
        for key, value in node.items():
            walk(value, f"{path}/{key}")
    elif isinstance(node, list):
        for index, value in enumerate(node):
            walk(value, f"{path}/{index}")


walk(spec, '#')

paths = spec.get('paths')
if not isinstance(paths, dict) or not paths:
    problem("paths is missing or empty")
else:
    for path, item in paths.items():
        if not isinstance(item, dict):
            problem(f"path {path} is not a mapping")
            continue
        for method, operation in item.items():
            if method not in HTTP_METHODS:
                continue
            if not isinstance(operation, dict):
                problem(f"{method.upper()} {path} is not a mapping")
                continue
            if not operation.get('responses'):
                problem(f"{method.upper()} {path} has no responses")

# security requirements must reference a declared securityDefinition
def check_security(requirements, where):
    if not isinstance(requirements, list):
        return
    for requirement in requirements:
        if not isinstance(requirement, dict):
            continue
        for name in requirement:
            if name not in security_definitions:
                problem(f"{where}: security scheme {name!r} is not declared in securityDefinitions")


check_security(spec.get('security'), 'top-level security')
for path, item in (paths or {}).items():
    if not isinstance(item, dict):
        continue
    for method, operation in item.items():
        if method in HTTP_METHODS and isinstance(operation, dict):
            check_security(operation.get('security'), f"{method.upper()} {path}")

# --- mirror sync: swagger.md's "## Version:" must match -----------------------
try:
    with open(mirror_path, encoding='utf-8') as handle:
        mirror = handle.read()
except FileNotFoundError:
    problem(f"mirror {mirror_path} not found")
else:
    mirror_version = re.search(r'^## Version:\s*(\S+)\s*$', mirror, re.MULTILINE)
    if mirror_version is None:
        problem("swagger.md has no '## Version:' line")
    elif mirror_version.group(1) != str(spec_version):
        problem(
            f"swagger.md declares Version {mirror_version.group(1)!r} but "
            f"swagger.yaml has {spec_version!r}")

if problems:
    print('swagger lint failed:', file=sys.stderr)
    for entry in problems:
        print('  - ' + entry, file=sys.stderr)
    sys.exit(1)

# Optional: full schema validation when the package is available.
try:
    from swagger_spec_validator.validator20 import validate_spec
    validate_spec(spec)
    print(f"swagger lint passed ({len(paths)} paths, {len(definitions)} definitions); full schema validation OK")
except ImportError:
    print(
        f"swagger lint passed ({len(paths)} paths, {len(definitions)} definitions); "
        "swagger-spec-validator not installed, structural checks only")
except Exception as error:  # noqa: BLE001 - report the validator's message
    print(f'swagger spec validation failed: {error}', file=sys.stderr)
    sys.exit(1)
PY
