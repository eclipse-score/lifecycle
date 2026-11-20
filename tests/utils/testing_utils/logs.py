# *******************************************************************************
# Copyright (c) 2026 Contributors to the Eclipse Foundation
#
# See the NOTICE file(s) distributed with this work for additional
# information regarding copyright ownership.
#
# This program and the accompanying materials are made available under the
# terms of the Apache License Version 2.0 which is available at
# https://www.apache.org/licenses/LICENSE-2.0
#
# SPDX-License-Identifier: Apache-2.0
# *******************************************************************************
"""Deserialization of DLT log files (storage format) written by the Launch Manager."""

from dataclasses import dataclass
from datetime import datetime, timezone
from pathlib import Path
from typing import List

# python-dlt from ITF
from dlt import dlt


@dataclass(frozen=True)
class LogMessage:
    """A single log message emitted by the Launch Manager (or its libraries)."""

    timestamp: datetime
    ecu_id: str
    app_id: str
    ctx_id: str
    level: str
    message: str


def _text(value) -> str:
    return (
        value.decode("ascii", errors="replace")
        if isinstance(value, bytes)
        else str(value)
    )


def parse_dlt_file(path: Path) -> List[LogMessage]:
    """Deserializes the log messages of the DLT file at `path`.

    Messages which are not logs (control or trace messages) are skipped.
    """
    messages = []
    for msg in dlt.load(str(path)):
        if _text(msg.type_string) != "log":
            continue
        header = msg.storageheader
        timestamp = datetime.fromtimestamp(header.seconds, tz=timezone.utc).replace(
            microsecond=max(header.microseconds, 0)
        )
        messages.append(
            LogMessage(
                timestamp=timestamp,
                ecu_id=_text(msg.headerextra.ecu).rstrip("\x00"),
                app_id=_text(msg.apid).rstrip("\x00"),
                ctx_id=_text(msg.ctid).rstrip("\x00"),
                level=_text(msg.subtype_string),
                message=_text(msg.payload_decoded),
            )
        )
    return messages
