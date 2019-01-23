#!/usr/bin/env bash

InterruptInterval=80000
VictimOn_core_id=3
PYTHON_EXEC="python3"

echo '
This demo shows how SGXligner attacks a program.
Note this demo does not configure the CPU frequency  
to a fixed speed, so you may observe the frequency
scalling effects in the produced graph.
'

echo -n "[INFO] Do some checking ... "
lsmod | grep "sgxlinger" &> /dev/null || { printf "\n[ERROR] sgxlinger kernel module not loaded!\n"; exit 1; }
hash "$PYTHON_EXEC" || { printf "\n[ERROR] this demo needs "$PYTHON_EXEC" !"; exit 1; }
$PYTHON_EXEC -c "import matplotlib" || { printf  "\n[ERROR] This demo needs matplotlib. Install it by command\\n\t\tsudo apt-get install python3-matplotlib\n"; exit 1; }
if [[ ! -e "../retrieve_sgxlinger_data/retrieve_data" ]]; then
    echo "[WARN] retrieve_data tool not found. Try compiling it ... "
    ( cd ../retrieve_sgxlinger_data; make; ) || { echo "FAIL!"; exit 1; }
fi
echo "Done."

echo -n "[Info] Set interrupt interval to $InterruptInterval ... "
if ! sudo -n true 2>/dev/null; then echo ""; fi # write a newline if we are about to prompt for sudo
echo "$InterruptInterval" | sudo tee "/sys/kernel/debug/sgxlinger/deadline_delta" >/dev/null
[[ $(sudo cat "/sys/kernel/debug/sgxlinger/deadline_delta") -eq $InterruptInterval ]] || { echo "FAIL!"; exit 1; }
echo "Done."

echo "[Info] Starting SGXlinger monitoring on victim.py scheduled on core $VictimOn_core_id ..."
echo 1 | sudo tee "/sys/kernel/debug/sgxlinger/enabled" >/dev/null
taskset --cpu-list "$VictimOn_core_id" $PYTHON_EXEC victim.py
echo 0 | sudo tee "/sys/kernel/debug/sgxlinger/enabled" >/dev/null

echo -n "[Info] SGXlinger finishes. Number of measurements collected: "
sudo cat "/sys/kernel/debug/sgxlinger/data_pos"

echo -n "[Info] Dumping measurement data ... "
sudo ../retrieve_sgxlinger_data/retrieve_data > data.log
echo "Done."

echo "[Info] Draw the interrupt latency timing graph ... "
$PYTHON_EXEC util_draw_data.py data.log &

echo "[Info] Cleanup the SGXlinger kernel module buffer."
echo "0" | sudo tee "/sys/kernel/debug/sgxlinger/data_pos" >/dev/null

