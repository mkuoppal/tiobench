#!/usr/bin/perl -w

#    Author: James Manning <jmm@computer.org>
#       This software may be used and distributed according to the terms of
#       the GNU General Public License, http://www.gnu.org/copyleft/gpl.html
#
#    Description:
#       Perl wrapper for calling the tiotest executable multiple times
#       with varying sets of parameters as instructed

use strict;
use Getopt::Long;

$|=1; # give output ASAP

sub usage {
   print "Usage: $0 [<options>]\n","Available options:\n\t",
            "[--help] (this help text)\n\t",
            "[--nofrag] (don't write fragmented files)\n\t",
            "[--size SizeInMB]+\n\t",
            "[--numruns NumberOfRuns]+\n\t",
            "[--dir TestDir]+\n\t",
            "[--block BlkSizeInBytes]+\n\t",
            "[--random NumberRandOpsPerThread]+\n\t",
            "[--threads NumberOfThreads]+\n\n",
   "+ means you can specify this option multiple times to cover multiple\n",
   "cases, for instance: $0 --block 4096 --block 8192 will first run\n",
   "through with a 4KB block size and then again with a 8KB block size.\n",
   "--numruns specifies over how many runs each test should be averaged\n";
   exit(1);
}

# look around for tiotest in different places
my @tiotest_places=(
   '.',                      # current directory
   '/usr/local/bin',         # install target location
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

# variables
my @sizes;       my $size;      my @dirs;    my $dir;
my @blocks;      my $block;     my @threads; my $thread;
my $random_ops;  my %stat_data; my $area;    my $mem_size;

my $write_mbytes;  my $write_time;  my $write_utime;  my $write_stime;
my $rwrite_mbytes; my $rwrite_time; my $rwrite_utime; my $rwrite_stime;
my $read_mbytes;   my $read_time;   my $read_utime;   my $read_stime;
my $rread_mbytes;  my $rread_time;  my $rread_utime;  my $rread_stime;
my $num_runs;      my $run_number;  my $help;         my $nofrag;

# option parsing
GetOptions("dir=s@",\@dirs,
           "size=i@",\@sizes,
           "block=i@",\@blocks,
           "random=i",\$random_ops,
           "numruns=i",\$num_runs,
           "help",\$help,
           "nofrag",\$nofrag,
           "threads=i@",\@threads);

&usage if $help || $Getopt::Long::error;

# give some default values
$num_runs=1 unless $num_runs && $num_runs > 0;
@dirs=qw(.) unless @dirs;
@blocks=qw(4096) unless @blocks;
@threads=qw(1 2 4 8) unless @threads;
$random_ops=4000 unless $random_ops;
unless(@sizes) { # try to be a little smart about file size when possible
   my $mem_size; my @stat_ret;
   if(@stat_ret = stat("/proc/kcore")) {
      $mem_size=int($stat_ret[7]/(1024*1024));
   } else { $mem_size=256; }           # default in case no kcore
   my $use_size=2*($mem_size);         # try to use at least twice memory
   $use_size=200  if $use_size < 200;  # min
   $use_size=2000 if $use_size > 2000; # max
   @sizes=($use_size);
   print "No size specified, using $use_size MB\n";
}

# setup the reporting stuff for fancy output
format REPORT_TOP =
Size is MB, BlkSz is Bytes, Read, Write, and Seeks are MB/sec

         File   Block  Num  Seq Read    Rand Read   Seq Write  Rand Write
  Dir    Size   Size   Thr Rate (CPU%) Rate (CPU%) Rate (CPU%) Rate (CPU%)
------- ------ ------- --- ----------- ----------- ----------- -----------
.

format REPORT =
@|||||| @||||| @|||||| @|| @>>>> @>>>% @>>>> @>>>% @>>>> @>>>% @>>>> @>>>%
$dir,$size,$block,$thread,$stat_data{'read'}{'rate'},$stat_data{'read'}{'cpu'},$stat_data{'rread'}{'rate'},$stat_data{'rread'}{'cpu'},$stat_data{'write'}{'rate'},$stat_data{'write'}{'cpu'},$stat_data{'rwrite'}{'rate'},$stat_data{'rwrite'}{'cpu'}
.

$-=0; $~='REPORT'; $^L=''; # reporting variables

# run all the possible combinations/permutations/whatever
foreach $dir (@dirs) {
   foreach $size (@sizes) {
      foreach $block (@blocks) {
         foreach $thread (@threads) {
            my $thread_rand=int($random_ops/$thread);
            my $thread_size=int($size/$thread);
            my $run_string = "$tiotest -t $thread -f $thread_size ".
                             "-r $thread_rand -b $block -d $dir -T";
            $run_string .= " -W" if $nofrag;
            foreach $run_number (1..$num_runs) {
               #my $prompt="Run #$run_number: $thread threads";
               my $prompt="Run #$run_number: $run_string";
               print STDERR $prompt;
               open(TIOTEST,"$run_string |") or die "Could not run $tiotest";

               while(<TIOTEST>) {
                  my ($field,$amount,$time,$utime,$stime)=split(/[:,]/);
                  $stat_data{$field}{'amount'} += $amount;
                  $stat_data{$field}{'time'}   += $time;
                  $stat_data{$field}{'utime'}  += $utime;
                  $stat_data{$field}{'stime'}  += $stime;
               }
               close(TIOTEST);
               print STDERR "" x length($prompt); # erase prompt
            }
            for my $field ('read','rread','write','rwrite') {
               $stat_data{$field}{'rate'} = 
                  $stat_data{$field}{'amount'} /
                  $stat_data{$field}{'time'};
               $stat_data{$field}{'cpu'} = 
                  100 * ( $stat_data{$field}{'utime'} +
                  $stat_data{$field}{'stime'} ) / 
                  $stat_data{$field}{'time'};
            }
            write;
         }
      }
   }
}
print STDERR "\n"; # look nicer for redir'd stdout
