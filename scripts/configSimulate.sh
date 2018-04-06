#!/bin/bash
# Allows batch runs of simulations. Saves results to log files

# run if user hits control-c
function control_c() {
    echo -en "*** Ouch! Exiting ***\n"
    exit $?
}

# Install WARPED-2 and build WARPED-2 models
# build <rootPath> <gitBranch> <mpiIncludePath> <mpiLibraryPath> <additionalFlags>
function build {
    rootPath=$1
    gitBranch=$2
    mpiIncludePath=$3
    mpiLibraryPath=$4
    additionalFlags=$5

    echo -e "\nInstalling WARPED-2:$gitBranch with additional flags $additionalFlags"

    cd $rootPath/warped2/
    git checkout $gitBranch
    autoreconf -i
    ./configure --with-mpi-includedir=$mpiIncludePath \
        --with-mpi-libdir=$mpiLibraryPath --prefix=$rootPath/installation/ $additionalFlags
    make
    make install

    cd $rootPath/warped2-models/
    autoreconf -i
    ./configure --with-warped=$rootPath/installation/
    make

    cd $rootPath/warped2-models/scripts/
}

# Create the Louvain partition profile for bags
# bagProfile <model> <modelCmd> <maxSimTime> <fileName>
function bagProfile {
    model=$1
    modelCmd=$2
    maxSimTime=$3
    fileName=$4

    cd ../models/$model/
    echo -e "\nProfile $modelCmd , File: $fileName , Max Sim Time: $maxSimTime"
    $modelCmd --max-sim-time $maxSimTime --simulation-type "sequential" \
            --sequential-statistics-type "louvain" --sequential-statistics-file $fileName

    cd ../../scripts/
}

# Run simulations for bag scheduling
# bagRun    <testCycles> <timeoutPeriod> <model> <modelCmd> <maxSimTime>
#           <workerThreads> <static_bag_window_size> <frac_bag_window>
#           <gvtMethod> <gvtPeriod> <stateSavePeriod> <partitioningFile>
function bagRun {
    testCycles=$1
    timeoutPeriod=$2
    model=$3
    modelCmd=$4
    maxSimTime=$5
    workerThreads=$6
    staticBagWindowSize=$7
    fracBagWindow=$8
    gvtMethod=$9
    gvtPeriod=$10
    stateSavePeriod=$11
    partitioningFile=$12

    logFile="logs/bags.csv"

    if [-e $logFile]
    then
        echo -e "Data will now be recorded in $logFile"
    else
        echo -e "New logfile $logFile created"
        header="Model,ModelCommand,MaxSimTime,WorkerThreadCount,StaticBagWindowSize,\
                FracBagWindow,GVTmethod,GVTperiod,StateSavePeriod,Runtime,NumObjects,\
                NumPartitions,LocalPositiveEventsSent,RemotePositiveEventsSent,\
                LocalNegativeEventsSent,RemoteNegativeEventsSent,PrimaryRollbacks,\
                SecondaryRollbacks,CoastForwardedEvents,CancelledEvents,EventsProcessed,\
                EventsCommitted,AvgMaxMemory"
        echo $header > $logFile
    fi

    for iteration in {1..$testCycles}
    do
        cd ../models/$model/
        outMsg="\n($iteration/$testCycles) $modelCmd : $workerThreads threads, \
                bags-$partitioningFile, static bag window size: $staticBagWindowSize, \
                fraction of bag window: $fracBagWindow, GVT: $gvtMethod-$gvtPeriod, \
                state saving period: $stateSavePeriod, max sim time: $maxSimTime"
        echo -e $outMsg
        tmpFile=`tempfile`
        runCommand="$modelCmd \
                    --max-sim-time $maxSimTime \
                    --time-warp-worker-threads $workerThreads \
                    --time-warp-gvt-calculation-method $gvtMethod \
                    --time-warp-gvt-calculation-period $gvtPeriod  \
                    --time-warp-state-saving-period $stateSavePeriod \
                    --time-warp-partitioning-file $partitioningFile \
                    --time-warp-statistics-file $tmpFile"

        timeout $timeoutPeriod bash -c "$runCommand" | grep -e "Simulation completed in "

        # Parse stats
        statsRaw=`cat $tmpFile | grep "Total,"`
        statsRefined=`echo $statsRaw | sed -e 's/Total,//g' -e 's/\t//g' -e 's/ //g'`

        rm $tmpFile
        cd ../../scripts/

        # Write to log file
        echo "$model,$modelCmd,$maxSimTime,$workerThreads,$staticBagWindowSize,\
                $fracBagWindow,$gvtMethod,$gvtPeriod,$stateSavePeriod,$statsRefined" >> $logFile
        sleep 10
    done
}

# Run simulations for chain scheduling
# chainRun  <testCycles> <timeoutPeriod> <model> <modelCmd>
#           <maxSimTime> <workerThreads> <scheduleQType>
#           <scheduleQCount> <chainSize> <isLpMigrationOn>
#           <gvtMethod> <gvtPeriod> <stateSavePeriod>
function chainRun {
    testCycles=$1
    timeoutPeriod=$2
    model=$3
    modelCmd=$4
    maxSimTime=$5
    workerThreads=$6
    scheduleQType=$7
    scheduleQCount=$8
    chainSize=$9
    isLpMigrationOn=$10
    gvtMethod=$11
    gvtPeriod=$12
    stateSavePeriod=$13

    logFile="logs/chains.csv"

    if [-e $logFile]
    then
        echo -e "Data will now be recorded in $logFile"
    else
        echo -e "New logfile $logFile created"
        header="Model,ModelCommand,MaxSimTime,WorkerThreadCount,ScheduleQType,ScheduleQCount,\
                ChainSize,isLPmigrationON,GVTmethod,GVTperiod,StateSavePeriod,Runtime,NumObjects,\
                LocalPositiveEventsSent,RemotePositiveEventsSent,LocalNegativeEventsSent,\
                RemoteNegativeEventsSent,PrimaryRollbacks,SecondaryRollbacks,\
                CoastForwardedEvents,CancelledEvents,EventsProcessed,EventsCommitted,AvgMaxMemory"
        echo $header > $logFile
    fi

    for iteration in {1..$testCycles}
    do
        cd ../models/$model/
        outMsg="\n($iteration/$testCycles) $modelCmd : $workerThreads threads, \
                $scheduleQCount $scheduleQType, chain size: $chainSize, \
                is LP migration on: $isLpMigrationOn, GVT: $gvtMethod-$gvtPeriod, \
                state saving period: $stateSavePeriod, max sim time: $maxSimTime"
        echo -e $outMsg
        tmpFile=`tempfile`
        runCommand="$modelCmd \
                    --max-sim-time $maxSimTime \
                    --time-warp-worker-threads $workerThreads \
                    --time-warp-scheduler-count $scheduleQCount \
                    --time-warp-group-size $chainSize \
                    --time-warp-lp-migration $isLpMigrationOn \
                    --time-warp-gvt-calculation-method $gvtMethod \
                    --time-warp-gvt-calculation-period $gvtPeriod  \
                    --time-warp-state-saving-period $stateSavePeriod \
                    --time-warp-statistics-file $tmpFile"

        timeout $timeoutPeriod bash -c "$runCommand" | grep -e "Simulation completed in " -e "Type of Schedule queue: "

        # Parse stats
        statsRaw=`cat $tmpFile | grep "Total,"`
        statsRefined=`echo $statsRaw | sed -e 's/Total,//g' -e 's/\t//g' -e 's/ //g'`

        rm $tmpFile
        cd ../../scripts/

        # Write to log file
        echo "$model,$modelCmd,$maxSimTime,$workerThreads,$scheduleQType,\
                $scheduleQCount,$chainSize,$isLpMigrationOn,$gvtMethod,\
                $gvtPeriod,$stateSavePeriod,$statsRefined" >> $logFile
        sleep 10
    done
}

# Run simulations for block scheduling
# blockRun  <testCycles> <timeoutPeriod> <model> <modelCmd>
#           <maxSimTime> <workerThreads> <scheduleQType>
#           <scheduleQCount> <blockSize> <isLpMigrationOn>
#           <gvtMethod> <gvtPeriod> <stateSavePeriod>
function blockRun {
    testCycles=$1
    timeoutPeriod=$2
    model=$3
    modelCmd=$4
    maxSimTime=$5
    workerThreads=$6
    scheduleQType=$7
    scheduleQCount=$8
    blockSize=$9
    isLpMigrationOn=$10
    gvtMethod=$11
    gvtPeriod=$12
    stateSavePeriod=$13

    logFile="logs/blocks.csv"

    if [-e $logFile]
    then
        echo -e "Data will now be recorded in $logFile"
    else
        echo -e "New logfile $logFile created"
        header="Model,ModelCommand,MaxSimTime,WorkerThreadCount,ScheduleQType,ScheduleQCount,\
                BlockSize,isLPmigrationON,GVTmethod,GVTperiod,StateSavePeriod,Runtime,NumObjects,\
                LocalPositiveEventsSent,RemotePositiveEventsSent,LocalNegativeEventsSent,\
                RemoteNegativeEventsSent,PrimaryRollbacks,SecondaryRollbacks,\
                CoastForwardedEvents,CancelledEvents,EventsProcessed,EventsCommitted,AvgMaxMemory"
        echo $header > $logFile
    fi

    for iteration in {1..$testCycles}
    do
        cd ../models/$model/
        outMsg="\n($iteration/$testCycles) $modelCmd : $workerThreads threads, \
                $scheduleQCount $scheduleQType, block size: $chainSize, \
                is LP migration on: $isLpMigrationOn, GVT: $gvtMethod-$gvtPeriod, \
                state saving period: $stateSavePeriod, max sim time: $maxSimTime"
        echo -e $outMsg
        tmpFile=`tempfile`
        runCommand="$modelCmd \
                    --max-sim-time $maxSimTime \
                    --time-warp-worker-threads $workerThreads \
                    --time-warp-scheduler-count $scheduleQCount \
                    --time-warp-group-size $blockSize \
                    --time-warp-lp-migration $isLpMigrationOn \
                    --time-warp-gvt-calculation-method $gvtMethod \
                    --time-warp-gvt-calculation-period $gvtPeriod  \
                    --time-warp-state-saving-period $stateSavePeriod \
                    --time-warp-statistics-file $tmpFile"

        timeout $timeoutPeriod bash -c "$runCommand" | grep -e "Simulation completed in " -e "Type of Schedule queue: "

        # Parse stats
        statsRaw=`cat $tmpFile | grep "Total,"`
        statsRefined=`echo $statsRaw | sed -e 's/Total,//g' -e 's/\t//g' -e 's/ //g'`

        rm $tmpFile
        cd ../../scripts/

        # Write to log file
        echo "$model,$modelCmd,$maxSimTime,$workerThreads,$scheduleQType,\
                $scheduleQCount,$blockSize,$isLpMigrationOn,$gvtMethod,\
                $gvtPeriod,$stateSavePeriod,$statsRefined" >> $logFile
        sleep 10
    done
}

# Run simulations for STL MultiSet, Splay, LadderQ, Unsorted Bottom (Locked and Lockfree)
# scheduleQRun  <testCycles> <timeoutPeriod> <model> <modelCmd>
#               <maxSimTime> <workerThreads> <scheduleQType>
#               <scheduleQCount> <isLpMigrationOn> <gvtMethod>
#               <gvtPeriod> <stateSavePeriod>
function scheduleQRun {
    testCycles=$1
    timeoutPeriod=$2
    model=$3
    modelCmd=$4
    maxSimTime=$5
    workerThreads=$6
    scheduleQType=$7
    scheduleQCount=$8
    isLpMigrationOn=$9
    gvtMethod=$10
    gvtPeriod=$11
    stateSavePeriod=$12

    logFile="logs/scheduleq.csv"

    if [-e $logFile]
    then
        echo -e "Data will now be recorded in $logFile"
    else
        echo -e "New logfile $logFile created"
        header="Model,ModelCommand,MaxSimTime,WorkerThreadCount,ScheduleQType,ScheduleQCount,\
                isLPmigrationON,GVTmethod,GVTperiod,StateSavePeriod,Runtime,NumObjects,\
                LocalPositiveEventsSent,RemotePositiveEventsSent,LocalNegativeEventsSent,\
                RemoteNegativeEventsSent,PrimaryRollbacks,SecondaryRollbacks,\
                CoastForwardedEvents,CancelledEvents,EventsProcessed,EventsCommitted,AvgMaxMemory"
        echo $header > $logFile
    fi

    for iteration in {1..$testCycles}
    do
        cd ../models/$model/
        outMsg="\n($iteration/$testCycles) $modelCmd : $workerThreads threads, \
                $scheduleQCount $scheduleQType, is LP migration on: $isLpMigrationOn, \
                GVT: $gvtMethod-$gvtPeriod, state saving period: $stateSavePeriod, \
                max sim time: $maxSimTime"
        echo -e $outMsg
        tmpFile=`tempfile`
        runCommand="$modelCmd \
                    --max-sim-time $maxSimTime \
                    --time-warp-worker-threads $workerThreads \
                    --time-warp-scheduler-count $scheduleQCount \
                    --time-warp-lp-migration $isLpMigrationOn \
                    --time-warp-gvt-calculation-method $gvtMethod \
                    --time-warp-gvt-calculation-period $gvtPeriod  \
                    --time-warp-state-saving-period $stateSavePeriod \
                    --time-warp-statistics-file $tmpFile"

        timeout $timeoutPeriod bash -c "$runCommand" | grep -e "Simulation completed in " -e "Type of Schedule queue: "

        # Parse stats
        statsRaw=`cat $tmpFile | grep "Total,"`
        statsRefined=`echo $statsRaw | sed -e 's/Total,//g' -e 's/\t//g' -e 's/ //g'`

        rm $tmpFile
        cd ../../scripts/

        # Write to log file
        echo "$model,$modelCmd,$maxSimTime,$workerThreads,$scheduleQType,\
                $scheduleQCount,$isLpMigrationOn,$gvtMethod,$gvtPeriod,\
                $stateSavePeriod,$statsRefined" >> $logFile
        sleep 10
    done
}

trap control_c SIGINT

. $1

