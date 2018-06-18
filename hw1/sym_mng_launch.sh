#!/bin/bash

# Concatenate correctly PATH_TO_DATA and DATA_FILE
PATH_TO_FILE="${PATH_TO_DATA}/${DATA_FILE}"

# Set full access rights to the user and cancel any rights to group and others
chmod -rwx,u+rwx ${PATH_TO_FILE}

# Launch the Manager at the background and quit
FULL_COMMAND="${FULL_EXE_NAME} ${PATH_TO_FILE} ${PATTERN} ${BOUND}"
$FULL_COMMAND &
exit 1
