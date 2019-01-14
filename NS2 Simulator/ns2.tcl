#Input from user
    set a [lindex $argv 0];
    set b [lindex $argv 1];

#Initialising netsim
    set ns [new Simulator];

#creating animator file
    set f [open output.nam w];
    $ns namtrace-all $f;

#creating trace dump file
    set nf [open test.tr w];
    $ns trace-all $nf;
    
     set throughputsrc1 0
    set throughputsrc2 0
    set netcounter 0

#defining "end" function
    proc end {} {
         global ns f nf throughputsrc1 throughputsrc2 netcounter;

         $ns flush-trace;
         puts "Avg throughput for Src1=[expr $throughputsrc1/$netcounter] MBits/sec\n"
         puts "Avg throughput for Src2=[expr $throughputsrc2/$netcounter] MBits/sec\n"
         close $f;
         close $nf;

         exec nam output.nam &;
         exit 0;
} 

#defining color coding
    $ns color 1 Red;
    $ns color 2 Blue;

#defining nodes
    set R1 [$ns node];
    set R2 [$ns node];
    set src1 [$ns node];
    set src2 [$ns node];
    set dst1 [$ns node];
    set dst2 [$ns node];

#defining links
    $ns duplex-link $R1 $R2 1mb 5ms DropTail;
    $ns duplex-link $src1 $R1 10mb 0ms DropTail;
    $ns duplex-link $dst1 $R2 10mb 0ms DropTail;

  if {$b == 1} {
   $ns duplex-link $src2 $R1 10mb 3.75ms DropTail;
   $ns duplex-link $dst2 $R2 10mb 3.75ms DropTail;
} elseif {$b == 2} {
   $ns duplex-link $src2 $R1 10mb 7.5ms DropTail;
   $ns duplex-link $dst2 $R2 10mb 7.5ms DropTail;
} elseif {$b == 3} {
   $ns duplex-link $src2 $R1 10mb 11.25ms DropTail;
   $ns duplex-link $dst2 $R2 10mb 11.25ms DropTail;
}

#Defining topology
    $ns duplex-link-op $R1 $R2 orient right;
    $ns duplex-link-op $src1 $R1 orient right-down;
    $ns duplex-link-op $dst1 $R2 orient right-up;
    $ns duplex-link-op $src2 $R1 orient right-up;
    $ns duplex-link-op $dst1 $R2 orient right-down;

#establishing tcp connections
  if {$a == "sack"} {
      set tcp [new Agent/TCP/Sack1];
      set tcp1 [new Agent/TCP/Sack1];
} elseif {$a == "vegas"} {
       set tcp [new Agent/TCP/Vegas];
       set tcp1 [new Agent/TCP/Vegas];
}

    $ns attach-agent $src2 $tcp1;
    $ns attach-agent $src1 $tcp;
 
    set tcpsink [new Agent/TCPSink];
    $ns attach-agent $dst1 $tcpsink;

    set tcpsink1 [new Agent/TCPSink];
    $ns attach-agent $dst2 $tcpsink1;
   
    $ns connect $tcp $tcpsink;
    $ns connect $tcp1 $tcpsink1;

#establishing ftp connections
    set ftp [new Application/FTP];
    $ftp attach-agent $tcp;

    set ftp1 [new Application/FTP];
    $ftp1 attach-agent $tcp1;

#Defining classes for color coding
    $tcp set class_ 1;
    $tcp1 set class_ 2;

#Recording data in output files
    set rcd1 [open record_1.tr w]
    set rcd2 [open record_2.tr w]
    
#record procedure to actually write data to output files
        proc record {} {
        global tcpsink tcpsink1 rcd1 rcd2 throughputsrc1 throughputsrc2 netcounter
        
#an instance of simulator
        set ns [Simulator instance]
        
#set time after which procedure will be called again
        set time 0.5

#get current time
        set now [$ns now]
        
#traffic received by sinks
        set bw1 [$tcpsink set bytes_]
        set bw2 [$tcpsink1 set bytes_]
               
# calculate the bandwidth (in MBit/s) and write it to the files
        puts $rcd1 "$now [expr $bw1/$time*8/1000000]"
        puts $rcd2 "$now [expr $bw2/$time*8/1000000]"
        set throughputsrc1 [expr $throughputsrc1+ $bw1/$time*8/1000000 ]
        set throughputsrc2 [expr $throughputsrc2+ $bw2/$time*8/1000000 ]
        set netcounter [expr $netcounter + 1]
        
#reset bytes_ on the sinks
        $tcpsink set bytes_ 0
        $tcpsink1 set bytes_ 0
        
# re-schedule the procedure
        $ns at [expr $now+$time] "record"
    }
    
#labelling nodes
    $ns at 0 "$src1 label source1";
    $ns at 0 "$src2 label source2";
    $ns at 0 "$R1 label Router1";
    $ns at 0 "$R2 label Router2";
    $ns at 0 "$dst1 label receiver1";
    $ns at 0 "$dst2 label receiver2";


#initialising testing
    
    $ns at 0 "$ftp start";
    $ns at 0 "$ftp1 start";
    $ns at 100 "record"
    $ns at 400 "$ftp stop";
    $ns at 400 "$ftp1 stop";
    $ns at 400 "end";

    $ns run;


