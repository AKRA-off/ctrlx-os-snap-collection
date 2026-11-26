#!/bin/bash

INTERNAL_DATA="${SNAP_COMMON}/data"
EXTERNAL_DATA="/media/sdb1/data"

# If external media is accessible and data hasn't been migrated yet,
# copy the initialized database from internal to external storage.
if [ -d "/media/sdb1" ]; then
    if [ -f "${INTERNAL_DATA}/mysql.ibd" ] && [ ! -f "${EXTERNAL_DATA}/mysql.ibd" ]; then
        mkdir -p "${EXTERNAL_DATA}"
        chmod 775 "${EXTERNAL_DATA}"
        if "${SNAP}/usr/bin/setpriv" \
                --clear-groups --reuid snap_daemon --regid root \
                -- cp -a "${INTERNAL_DATA}/." "${EXTERNAL_DATA}/"; then
            chown snap_daemon:root "${EXTERNAL_DATA}"
            chmod 700 "${EXTERNAL_DATA}"
            # Clean up internal data only after successful copy.
            "${SNAP}/usr/bin/setpriv" \
                --clear-groups --reuid snap_daemon --regid root \
                -- rm -rf "${INTERNAL_DATA:?}/"*
        fi
    fi
fi

# Use external datadir if available, otherwise fall back to internal.
if [ -f "${EXTERNAL_DATA}/mysql.ibd" ]; then
    DATADIR="${EXTERNAL_DATA}"
elif [ -f "${INTERNAL_DATA}/mysql.ibd" ]; then
    DATADIR="${INTERNAL_DATA}"
else
    echo "ERROR: No initialized MySQL data directory found." >&2
    exit 1
fi

exec "${SNAP}/usr/bin/setpriv" \
    --clear-groups \
    --reuid snap_daemon \
    --regid root \
    -- \
    "${SNAP}/usr/sbin/mysqld" --defaults-file="${SNAP}/etc/my.cnf" --datadir="${DATADIR}"
