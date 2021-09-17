#!/usr/bin/env perl

#    Author: James Manning <jmm at users.sf.net>
#    Author: Randy Hron <rwhron at earthlink dot net>
#       This software may be used and distributed according to the terms of
#       the GNU General Public License, http://www.gnu.org/copyleft/gpl.html
#
#    Description:
#       Perl wrapper for calling the tiotest executable multiple times
#       with varying sets of parameters as instructed

use warnings;
use strict;
use Getopt::Long;

$|=1; # give output ASAP

# look around for tiotest in different places
my @tiotest_places=(
   '.',                      # current directory
   '/usr/local/bin',         # install target location
   '/usr/sbin',              # install target location
   split(':',$ENV{'PATH'}),  # directories in current $PATH
   ($0 =~m#(.*)/#)           # directory this script resides in
);
my $tiotest='';

foreach my $place (@tiotest_places) {
   $tiotest=$place . '/tiotest';
   last if -x $tiotest;
}

if (! -x $tiotest) {
    print "tiotest program not found in any of the following places:\n\n",
          join(' ',@tiotest_places),"\n\n",
          "copy it to one of them or modify this perl script's ",
          "\@tiotest_places array\n";
    exit(1);
}

### CONSTANTS
# sizes
my $KB = 1024;
my $MB = 1024 * 1024;
my $GB = 1024 * 1024 * 1024;

# debug levels, ala log4j and jakarta commons logging
my $LEVEL_NONE  = 0;
my $LEVEL_FATAL = 10;
my $LEVEL_ERROR = 20;
my $LEVEL_WARN  = 30;
my $LEVEL_INFO  = 40;
my $LEVEL_DEBUG = 50;
my $LEVEL_TRACE = 60;

### VARIABLES
my @sizes;       my $size;      my @targets; my $dir;
my @blocks;      my $block;     my @threads; my $thread;
my $random_ops;  my %stat_data; my $area;    my $mem_size;

my $write_mbytes;  my $write_time;  my $write_utime;  my $write_stime;
my $rwrite_mbytes; my $rwrite_time; my $rwrite_utime; my $rwrite_stime;
my $read_mbytes;   my $read_time;   my $read_utime;   my $read_stime;
my $rread_mbytes;  my $rread_time;  my $rread_utime;  my $rread_stime;
my $num_runs;      my $run_number;  my $help;         my $nofrag;
my $identifier;    my $debug;       my $dump;         my $progress;
my $timeout;       my $rawdev;      my $flushCaches;  my $directIO;

# option parsing
GetOptions("target=s@",\@targets,
           "identifier=s",\$identifier,
           "size=i@",\@sizes,
           "block=i@",\@blocks,
           "random=i",\$random_ops,
           "numruns=i",\$num_runs,
           "help",\$help,
           "nofrag",\$nofrag,
           "debug=i",\$debug,
           "timeout=i",\$timeout,
           "dump",\$dump,
           "progress",\$progress,
           "threads=i@",\@threads,
           "flushCaches", \$flushCaches,
           "directio", \$directIO,);

&usage if $help || $Getopt::Long::error;

my $start_time = time if $timeout;

### DEFAULT VALUES
$num_runs=1 unless $num_runs && $num_runs > 0;
@targets=qw(.) unless @targets;
@blocks=qw(4096) unless @blocks;
@threads=qw(1 2 4 8) unless @threads;
$random_ops=4000 unless $random_ops;
chomp($identifier=`uname -r`) unless $identifier;
$debug=$LEVEL_NONE unless defined($debug);
unless(@sizes) { # try to be a little smart about file size when possible
   my $mem_size = &get_memory_size();
   $mem_size = int($mem_size/$MB); # convert to MB
   my $use_size=2*($mem_size);         # try to use at least twice memory

   # used to cap the value between 200 MB and about 2 GB
   #$use_size=200  if $use_size < 200;  # min
   #$use_size=2000 if $use_size > 2000; # max

   @sizes=($use_size);
   print "No size specified, using $use_size MB\n"
      if $debug >= $LEVEL_INFO;
}

# setup the reporting stuff for fancy output
format SEQ_READS_TOP =
                              File   Blk   Num                         Avg      Maximum      Lat%     Lat%     CPU
Identifier                    Size   Size  Thr      Rate     (CPU%)  Latency    Latency      >2s      >10s     Eff
---------------------------- ------ ------ ---  ------------ ------ --------- -----------  -------- -------- -------
.

format SEQ_READS =
@<<<<<<<<<<<<<<<<<<<<<<<<<<< @||||| @||||| @>>  @########.## @>>>>% @####.### @#######.##  @#.##### @#.##### @######
$identifier,$size,$block,$thread,$stat_data{$identifier}{$thread}{$size}{$block}{'read'}{'rate'},$stat_data{$identifier}{$thread}{$size}{$block}{'read'}{'cpu'},$stat_data{$identifier}{$thread}{$size}{$block}{'read'}{'avglat'},$stat_data{$identifier}{$thread}{$size}{$block}{'read'}{'maxlat'},$stat_data{$identifier}{$thread}{$size}{$block}{'read'}{'pct_gt_2_sec'},$stat_data{$identifier}{$thread}{$size}{$block}{'read'}{'pct_gt_10_sec'},$stat_data{$identifier}{$thread}{$size}{$block}{'read'}{'cpueff'}
.

format RAND_READS =
@<<<<<<<<<<<<<<<<<<<<<<<<<<< @||||| @||||| @>>  @########.## @>>>>% @####.### @#######.##  @#.##### @#.##### @######
$identifier,$size,$block,$thread,$stat_data{$identifier}{$thread}{$size}{$block}{'rread'}{'rate'},$stat_data{$identifier}{$thread}{$size}{$block}{'rread'}{'cpu'},$stat_data{$identifier}{$thread}{$size}{$block}{'rread'}{'avglat'},$stat_data{$identifier}{$thread}{$size}{$block}{'rread'}{'maxlat'},$stat_data{$identifier}{$thread}{$size}{$block}{'rread'}{'pct_gt_2_sec'},$stat_data{$identifier}{$thread}{$size}{$block}{'rread'}{'pct_gt_10_sec'},$stat_data{$identifier}{$thread}{$size}{$block}{'rread'}{'cpueff'}
.

format SEQ_WRITES =
@<<<<<<<<<<<<<<<<<<<<<<<<<<< @||||| @||||| @>>  @########.## @>>>>% @####.### @#######.##  @#.##### @#.##### @######
$identifier,$size,$block,$thread,$stat_data{$identifier}{$thread}{$size}{$block}{'write'}{'rate'},$stat_data{$identifier}{$thread}{$size}{$block}{'write'}{'cpu'},$stat_data{$identifier}{$thread}{$size}{$block}{'write'}{'avglat'},$stat_data{$identifier}{$thread}{$size}{$block}{'write'}{'maxlat'},$stat_data{$identifier}{$thread}{$size}{$block}{'write'}{'pct_gt_2_sec'},$stat_data{$identifier}{$thread}{$size}{$block}{'write'}{'pct_gt_10_sec'},$stat_data{$identifier}{$thread}{$size}{$block}{'write'}{'cpueff'}
.


format RAND_WRITES =
@<<<<<<<<<<<<<<<<<<<<<<<<<<< @||||| @||||| @>>  @########.## @>>>>% @####.### @#######.##  @#.##### @#.##### @######
$identifier,$size,$block,$thread,$stat_data{$identifier}{$thread}{$size}{$block}{'rwrite'}{'rate'},$stat_data{$identifier}{$thread}{$size}{$block}{'rwrite'}{'cpu'},$stat_data{$identifier}{$thread}{$size}{$block}{'rwrite'}{'avglat'},$stat_data{$identifier}{$thread}{$size}{$block}{'rwrite'}{'maxlat'},$stat_data{$identifier}{$thread}{$size}{$block}{'rwrite'}{'pct_gt_2_sec'},$stat_data{$identifier}{$thread}{$size}{$block}{'rwrite'}{'pct_gt_10_sec'},$stat_data{$identifier}{$thread}{$size}{$block}{'rwrite'}{'cpueff'}
.



my $total_runs;
my $total_runs_completed;
my $progressbar;

if ($progress) {
   $total_runs = $num_runs    *
                 scalar(@targets) *
                 scalar(@sizes) *
                 scalar(@blocks) *
                 scalar(@threads);
   $total_runs_completed = 0;

   require Term::ProgressBar;
   $progressbar = Term::ProgressBar->new({name  => 'tiotest runs',
                                          count => $total_runs,
                                          ETA   => 'linear',
                                         });
}

my $targets_str = join '', map { " -d " . $_ } @targets;
if(&all_devices(@targets)) {
   print "All targets are devices\n"
      if $debug >= $LEVEL_INFO;
   $targets_str .= ' -R'; # add "raw devices" param
} elsif(&all_directories(@targets)) {
   print "All targets are directories\n"
      if $debug >= $LEVEL_INFO;
   # nothing to do here
} else {
   print STDERR "--target params must be either all devices or all directories\n";
   exit(1);
}

# run all the possible combinations/permutations/whatever
OUTER:
foreach $size (@sizes) {
   foreach $block (@blocks) {
      foreach $thread (@threads) {
         my $thread_rand=int($random_ops/$thread);
         my $thread_size=int($size/$thread); $thread_size=1 if $thread_size==0;
         my $run_string = "$tiotest -t $thread -f $thread_size ".
                          "-r $thread_rand -b $block $targets_str -T";
         $run_string .= " -W" if $nofrag;
         $run_string .= " -R" if $rawdev;
         $run_string .= " -D $debug" if $debug > 0;
         $run_string .= " -F" if $flushCaches;
         $run_string .= " -X" if $directIO;
         foreach $run_number (1..$num_runs) {
            print "Running: $run_string\n"
               if $debug >= $LEVEL_INFO;
            open(TIOTEST,"$run_string |") or die "Could not run $tiotest";

            while(my $line = <TIOTEST>) {
               next if $line =~ /^total/o; # this may be useful, but it's been ignored up to this point.
               print "Processing output line of $line"
                  if $debug >= $LEVEL_INFO;
               my ($field,$amount,$time,$utime,$stime,$avglat,$maxlat,$pct_gt_2_sec,$pct_gt_10_sec)=split(/[:,]/, $line);
               $stat_data{$identifier}{$thread}{$size}{$block}{$field}{'amount'} += $amount;
               $stat_data{$identifier}{$thread}{$size}{$block}{$field}{'time'}   += $time;
               $stat_data{$identifier}{$thread}{$size}{$block}{$field}{'utime'}  += $utime;
               $stat_data{$identifier}{$thread}{$size}{$block}{$field}{'stime'}  += $stime;
               $stat_data{$identifier}{$thread}{$size}{$block}{$field}{'avglat'} += $avglat;
               $stat_data{$identifier}{$thread}{$size}{$block}{$field}{'maxlat'} += $maxlat;
               $stat_data{$identifier}{$thread}{$size}{$block}{$field}{'pct_gt_2_sec'}  += $pct_gt_2_sec;
               $stat_data{$identifier}{$thread}{$size}{$block}{$field}{'pct_gt_10_sec'} += $pct_gt_10_sec;
            }
            close(TIOTEST);
            $progressbar->update(++$total_runs_completed) if $progress;
         }
         for my $field ('read','rread','write','rwrite') {
            $stat_data{$identifier}{$thread}{$size}{$block}{$field}{'rate'} =
               $stat_data{$identifier}{$thread}{$size}{$block}{$field}{'amount'} /
               $stat_data{$identifier}{$thread}{$size}{$block}{$field}{'time'};
            $stat_data{$identifier}{$thread}{$size}{$block}{$field}{'cpu'} =
               100 * ( $stat_data{$identifier}{$thread}{$size}{$block}{$field}{'utime'} +
               $stat_data{$identifier}{$thread}{$size}{$block}{$field}{'stime'} ) /
               $stat_data{$identifier}{$thread}{$size}{$block}{$field}{'time'};
            $stat_data{$identifier}{$thread}{$size}{$block}{$field}{'cpueff'} =
               ($stat_data{$identifier}{$thread}{$size}{$block}{$field}{'rate'} /
               ($stat_data{$identifier}{$thread}{$size}{$block}{$field}{'cpu'}/100+0.00001));
         }
         if($timeout && (time > ($start_time + $timeout))) {
            print STDERR "\nTimeout of $timeout seconds has been reached, aborting ($total_runs_completed of $total_runs runs completed)\n";
            last OUTER;
         }
      }
   }
}

if ($dump) {
   require Data::Dumper;
   print Data::Dumper->Dump([\%stat_data], [qw(stat_data)]);
   exit(0);
}

# report summary
print "
Unit information
================
File size = megabytes (2^20 bytes, 1,048,576 bytes)
Blk Size  = bytes
Rate      = megabytes per second
CPU%      = percentage of CPU used during the test
Latency   = milliseconds
Lat%      = percent of requests that took longer than X seconds
CPU Eff   = Rate divided by CPU% - throughput per cpu load
";

my %report = (
   'SEQ_READS'   => 'Sequential Reads',
   'RAND_READS'  => 'Random Reads',
   'SEQ_WRITES'  => 'Sequential Writes',
   'RAND_WRITES' => 'Random Writes',
);

# The top is the same for all 4 reports
$^ = 'SEQ_READS_TOP';

foreach my $title ('SEQ_READS', 'RAND_READS', 'SEQ_WRITES', 'RAND_WRITES') {
   $-=0; $~="$title"; $^L=''; # reporting variables
   print "\n$report{$title}\n";
   print '=' x length("$report{$title}") . "\n";
   foreach $size (@sizes) {
      foreach $block (@blocks) {
         foreach $thread (@threads) {
            write if defined($stat_data{$identifier}{$thread}{$size}{$block});
         }
      }
   }
}

###########################################
######### Utility subroutines #############
###########################################

sub usage {
   print "Usage: $0 [<options>]\n","Available options:\n\t",
            "[--help] (this help text)\n\t",
            "[--identifier IdentString] (use IdentString as identifier in output)\n\t",
            "[--nofrag] (don't write fragmented files)\n\t",
            "[--raw] (use raw device defined with --dir)\n\t",
            "[--size SizeInMB]+\n\t",
            "[--numruns NumberOfRuns]\n\t",
            "[--target DirOrDisk]+\n\t",
            "[--block BlkSizeInBytes]+\n\t",
            "[--random NumberRandOpsAllThreads]+\n\t",
            "[--threads NumberOfThreads]+\n\t",
            "[--dump] (dump in Data::Dumper format, no report)\n\t",
            "[--progress] (monitor progress with Term::ProgressBar)\n\t",
            "[--timeout TimeoutInSeconds]\n\t",
            "[--flushCaches] (requires root)\n\t",
            "[--debug DebugLevel]\n\n",
   "+ means you can specify this option multiple times to cover multiple\n",
   "cases, for instance: $0 --block 4096 --block 8192 will first run\n",
   "through with a 4KB block size and then again with a 8KB block size.\n",
   "\n",
   "--numruns specifies over how many runs each test combination of\n",
   "parameters should be averaged\n";
   "\n",
   "--target parameters are all tested in parallel on each tiotest\n",
   "run using tiotest's ability to take multiple -d parameters\n",
   exit(1);
}

# sub to try various methods to get the amount of memory of this machine
# returned value is in bytes
sub get_memory_size {
   my $mem_size; my @stat_ret;
   # first try /proc/meminfo if it's around and readable
   if(-r '/proc/meminfo') {
      open(MEMINFO,'/proc/meminfo') or die 'Error opening /proc/meminfo';
      my $line = (grep {/MemTotal:/} <MEMINFO>)[0];
      close(MEMINFO);
      print "Fetched line from /proc/meminfo of $line"
         if $debug >= $LEVEL_TRACE;
      my (undef, $amt, $unit) = split(/\s+/, $line);
      if    ($unit =~ /^kb$/i) { $amt *= $KB; }
      elsif ($unit =~ /^mb$/i) { $amt *= $MB; }
      elsif ($unit =~ /^gb$/i) { $amt *= $GB; }
      else { die "Do not understand the units in /proc/meminfo of $unit"; }
      print "Found memory size of $amt bytes from /proc/meminfo\n"
         if $debug >= $LEVEL_DEBUG;
      return $amt;
   # then try the size of /proc/kcore if that's available
   } elsif(-s '/proc/kcore') {
      my @stat_ret = stat("/proc/kcore");
      my $amt = $stat_ret[7];
      print "Found memory size of $amt bytes from size of /proc/kcore\n"
         if $debug >= $LEVEL_DEBUG;
      return $amt;
   # nothing else worked, just pick a default of 256 MB
   } else {
      print "Cannot figure out memory size, going with 256 MB\n"
         if $debug >= $LEVEL_DEBUG;
      return 256*$MB;
   }
}

sub all_devices {
   for my $path (@_) {
      print "Checking potential device $path\n"
         if $debug >= $LEVEL_TRACE;
      if(! -b $path && ! -c $path) {
         print "$path is not a device\n"
            if $debug >= $LEVEL_DEBUG;
         return 0;
      }
   }
   return 1;
}

sub all_directories {
   for my $path (@_) {
      print "Checking potential directory $path\n"
         if $debug >= $LEVEL_TRACE;
      if(! -d $path) {
         print "$path is not a directory\n"
            if $debug >= $LEVEL_DEBUG;
         return 0;
      }
   }
   return 1;
}
