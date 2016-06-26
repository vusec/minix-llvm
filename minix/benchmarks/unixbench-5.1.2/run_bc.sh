#/bin/sh
: ${UB_ROOT=$(pwd)}
: ${TMP=${UB_ROOT}/tmp}
: ${PGMS=${UB_ROOT}/pgms}
: ${LOG=${UB_ROOT}/results}

# Tests to run
: ${TESTS="dhry2reg whetstone-double execl fstime fsbuffer fsdisk pipe context1 spawn syscall shell1 shell8"}

: ${FILE_0=dhry2reg}
: ${FILE_1=whetstone-double}
: ${FILE_2=execl}
: ${FILE_3=fstime}
: ${FILE_4=fstime}
: ${FILE_5=fstime}
: ${FILE_6=pipe}
: ${FILE_7=context1}
: ${FILE_8=spawn}
: ${FILE_9=syscall}
: ${FILE_10=looper}
: ${FILE_11=looper}

# Number of time to run each test
: ${RUNS_0=10}
: ${RUNS_1=10}
: ${RUNS_2=3}
: ${RUNS_3=3}
: ${RUNS_4=3}
: ${RUNS_5=3}
: ${RUNS_6=10}
: ${RUNS_7=10}
: ${RUNS_8=3}
: ${RUNS_9=10}
: ${RUNS_10=3}
: ${RUNS_11=3}

# Arguments to be passed to each test
: ${ARGS_0=10}
: ${ARGS_1=''}
: ${ARGS_2=30}
: ${ARGS_3="-c -t 30 -d $TMP -b 1024 -m 2000"}
: ${ARGS_4="-c -t 30 -d $TMP -b 256 -m 500"}
: ${ARGS_5="-c -t 30 -d $TMP -b 4096 -m 8000"}
: ${ARGS_6=10}
: ${ARGS_7=10}
: ${ARGS_8=30}
: ${ARGS_9=10}
: ${ARGS_10="60 $PGMS/multi.sh 1"}
: ${ARGS_11="60 $PGMS/multi.sh 8"}

sequence=0
num_runs=`expr $RUNS_0 + $RUNS_1 + $RUNS_2 + $RUNS_3 + $RUNS_4 + $RUNS_5 + $RUNS_6 + $RUNS_7 + $RUNS_8 + $RUNS_9 + $RUNS_10 + $RUNS_11`
progress=0
 
hyper() {
	if [ "$HYPER" != "" ]; then
		"$HYPER" "$@"
	fi
}

print_progress()
{
exagg=5
        if [ "$1" != "print" ]; then
		return
	fi
        shift
	if echo "$@" | grep -q "Starting.*" ; then                                                  
           remaining=$(($num_runs * $exagg ))                                                  
           midstr=`printf "%0.s " \`seq 1 ${remaining}\``                                      
           printf "Progress  [%s]  %d/%d \r" "${midstr}" 0 ${num_runs}                         
           return                                                                              
        fi                                                                                          
        if echo "$@" | grep -q ".*: passed" ; then                                                  
           progress=`expr $progress + 1`                                                       
           remaining=$((((${num_runs} - ${progress}) * $exagg ) ))                             
           num_syms=$((${progress} * $exagg))                                                  
           prog_str=`printf "%0.s*" \`seq 1 ${num_syms}\``
	   if [ ${remaining} -eq 0 ]; then                                                     
                 space_str=                                                                  
           else                                                                                
                 space_str=`printf "%0.s " \`seq 1 ${remaining}\``                           
           fi                                                                                  
           printf "Progress : [%s%s]  %d/%d\r" "$prog_str" "$space_str" $progress $num_runs
	   return 
        fi            
	if echo "$@" | grep -q "Completed.*" ; then		
		printf "\n%-75s\n" "$@"
	fi
}

console_print()
{
        if [ "$1" != "print" ]; then
		return
	fi
        shift
        printf "%-75s\n" "$@"
}

hyperprintlines() {
	while read line; do
		hyper print "$*: $line"
	done
}

TESTSREPEATED=""
n=0
for t in $TESTS
do
    runs=`eval echo "\\${RUNS_$n}"`
    for i in `seq 1 $runs`; do
        TESTSREPEATED="$TESTSREPEATED $t"
    done
    n=$(expr $n + 1)
done

hyper print "Starting tests: $TESTSREPEATED"

#run_test(test indice, number of run, executable name, arguments)
run_test() {
    number=$1
    runs=$2
    name=$3

    file=`eval echo "\\${FILE_$number}"`
    args=`eval echo "\\${ARGS_$number}"`
    log="$LOG/$date-$sequence-$name-$number.log"

    #echo "log $log"
    #echo "number $number"
    #echo "runs $runs"
    #echo "name $name"
    #echo "args $args"

    while test $runs -gt 0
    do
	sync
	sleep 1
	sync
	sleep 2
	SDATE=$(date "+%s")
	hyper print "Test $name: running"
	hyper dump test-start-$name
	$PGMS/$file $args >$log.tmp 2>&1
	hyper dump test-done-$name
	hyper print "Test $name: passed"
	EDATE=$(date "+%s")
	hyperprintlines "Test $name: output line" < $log.tmp
	cat $log.tmp >> $log
	echo "ELAPSED|"$(expr $EDATE - $SDATE)"|s" >>$log
	runs=$(expr $runs - 1)
    done
}

# Initialisations
export UB_BINDIR="$PGMS"
export UB_TMPDIR="$TMP"
export UB_RESULTDIR="$LOG"
export UB_TESTDIR="$UB_ROOT/testdir"

date=$(date +%Y.%m.%d)
# A bit hackish, assumes dhry2reg is run as test 0...
while [ -e "$LOG/$date-$sequence-dhry2reg-0.log" ]
do
    sequence=$(expr $sequence + 1)
done

# Execute the tests
cd $UB_TESTDIR

n=0
for t in $TESTS
do
    runs=`eval echo "\\${RUNS_$n}"`
    run_test $n $runs $t
    n=$(expr $n + 1)
done

hyper print "Completed tests"
