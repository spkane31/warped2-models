# Synthetic Simulation Model #

Authors : Jingbin Yang, Sounak Gupta <br>
High Performance Computing Laboratory <br>
Dept of ECECS, PO Box 210030 <br>
Cincinnati, OH  45221--0030 <br>


## Introduction : ##

This simulation model allows the user to create synthetic approximation of different
categories of simulation models. A configurable set of simulation parameters allows
the user to manipulate the model's underlying network structure as well as rate and
distribution of event flow.


## Features : ##

Individual or combined effect(s) of the following features on simulation can be
studied using different configurations of this model :

    1. Total number of nodes (entites) which collectively form the simulation space.

    2. Structure of network connectivity between nodes.

    3. Rate of events sent from a node to its adjacent nodes.

    4. Distribution of events sent from a node to its adjacent nodes.

    5. Time to process an event.

    6. Dynamic change in size of the LP state.

## Configuration : ##

User can adjust the following parameters:

    1.  Number of nodes
            --num-nodes 100000 (default)

    2.  Network details (the communication links between nodes)
            --network-params <network-type,<network-params>>

            Flat-Mesh,1000,100 (default)
                rows : number of rows in the mesh
                cols : number of cols in the mesh

            Watts-Strogatz,30,0.1 (default)
                k : mean degree of connectivity (n >> k >> ln(k) >> 1)
                b : beta probability of link re-ordering

            Barabasi-Albert,30,0.1
                m : degree of initially connected network (>= 2)
                a : alpha probability of preferential attachment or bias

    3.  Node selection details (for sending inter-node events)
            --node-selection-params <distribution-type,<distribution-params>>

            exponential,0.5 (default)
                lambda : average rate of occurance (> 0)

            geometric,0.5
                p : probability of success

            binomial,0.5
                p : probability of success

            normal,5,10
                distribution mean
                standard deviation

            uniform,1,10
                a : lower bound of range
                b : upper bound of range

            poisson,9
                mean (> 3)

            lognormal,3,5
                mean
                standard deviation

    4.  Floating point operations count (used to set an event processing delay)
            --event-processing-time-range 1000,1000 (default)
            Change the values of <min,max>

    5.  State size (size of the LP state)
            --state-size-range 100,100 (default)
            Change the values of <min,max>

    6.  State size change range (dynamic state size)
            --state-size-change-range 0,0 (default)
            State size change within range of (-1,1)

    7.  Event send distribution (set the send time delta)
            --event-send-time-delta-params <distribution-type,<distribution-params>,ceiling>

            ceiling : the upper bound value, set to 10 in the following distribuitons

            geometric,0.1,10 (default)
                p : probability of success

            exponential,0.5,10
                lambda : average rate of occurance (> 0)

            binomial,0.5,10
                p : probability of success

            normal,5,9,10
                distribution mean
                standard deviation

            uniform,1,9,10
                a : lower bound of range
                b : upper bound of range

            poisson,9,10
                mean (> 3)

            lognormal,3,5,10
                mean
                standard deviation


## References : ##

