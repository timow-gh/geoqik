#!/usr/bin/env bash

set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
package_dir="${1:-${repo_root}/out/packages/package-gcc-release}"
consumer_compiler="${2:-g++-14}"
work_dir="${repo_root}/out/package-verification/linux"
consumer_source="${repo_root}/test/TestInstall/geoqik_install_test_template"

rm -rf "${work_dir}"
mkdir -p "${work_dir}"

shopt -s nullglob
archives=("${package_dir}"/*.tar.gz)
runtime_debs=("${package_dir}"/libgeoqik[0-9]*_*.deb)
dev_debs=("${package_dir}"/libgeoqik-dev_*.deb)

if [[ ${#archives[@]} -ne 1 || ${#runtime_debs[@]} -ne 1 || ${#dev_debs[@]} -ne 1 ]]; then
    printf 'Expected one TGZ, one runtime DEB, and one development DEB in %s\n' "${package_dir}" >&2
    exit 1
fi

verify_checksum() {
    local package="$1"
    local checksum=""
    for candidate in "${package}.sha256" "${package}.SHA256"; do
        if [[ -f "${candidate}" ]]; then
            checksum="${candidate}"
            break
        fi
    done
    if [[ -z "${checksum}" ]]; then
        printf 'Missing SHA-256 checksum for %s\n' "${package}" >&2
        exit 1
    fi
    (cd "${package_dir}" && sha256sum --check "$(basename "${checksum}")")
}

verify_checksum "${archives[0]}"
verify_checksum "${runtime_debs[0]}"
verify_checksum "${dev_debs[0]}"

verify_prefix() {
    local prefix="$1"
    local name="$2"
    local build_dir="${work_dir}/consumer-${name}"

    test -f "${prefix}/include/GeoQik/GeoQik.hpp"
    test -f "${prefix}/include/GeoQikClient/GeoQikClient.hpp"
    test -f "${prefix}/lib/cmake/geoqik/geoqikConfig.cmake"
    test -f "${prefix}/lib/libgeoqik.so"
    test -x "${prefix}/bin/geoqik_server"
    test -f "${prefix}/share/doc/geoqik/License"

    cmake -S "${consumer_source}" -B "${build_dir}" -G Ninja \
        -DCMAKE_CXX_COMPILER="${consumer_compiler}" \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_PREFIX_PATH="${prefix}"
    cmake --build "${build_dir}" --parallel

    PATH="${prefix}/bin:${PATH}" "${build_dir}/use_installdir"
    PATH="${prefix}/bin:${PATH}" "${build_dir}/use_installdir_client"
}

archive_dir="${work_dir}/archive"
mkdir -p "${archive_dir}"
tar -xzf "${archives[0]}" -C "${archive_dir}"
archive_roots=("${archive_dir}"/*)
if [[ ${#archive_roots[@]} -ne 1 || ! -d "${archive_roots[0]}" ]]; then
    printf 'Expected the TGZ to contain one package root\n' >&2
    exit 1
fi
verify_prefix "${archive_roots[0]}" archive

depends="$(dpkg-deb --field "${runtime_debs[0]}" Depends)"
if [[ -z "${depends}" ]]; then
    printf 'Runtime DEB has no dependency metadata\n' >&2
    exit 1
fi

deb_root="${work_dir}/deb"
mkdir -p "${deb_root}"
dpkg-deb --extract "${runtime_debs[0]}" "${deb_root}"
dpkg-deb --extract "${dev_debs[0]}" "${deb_root}"
verify_prefix "${deb_root}/usr" deb

printf 'Verified TGZ and DEB packages in %s\n' "${package_dir}"
