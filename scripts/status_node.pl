#!/usr/bin/perl -w
use IO::Socket;

print "\n Get the status of the Ka duplication process\n";
print " If you want to execute a command on node, just use the 'exec' prefix\n";
$node = shift(@ARGV);
#$node = "10.0.1.35";
! defined $node and die "ERROR: Enter a node address\n\n";
#print "Node name or IP:\n";
#$node = <>; chomp($node);

$socket = IO::Socket::INET->new(
    PeerAddr => $node,
    Proto => "tcp",
    PeerPort => 12345
    )
    or die "Can't connect to $node.\n";

$pid = fork;
die "Can't fork !" if ! defined ($pid);

if ($pid ==0) {
    while ($data = <>) {
	print $socket $data;
    }
} else {
    while ($data = <$socket>) {
	print "$node> $data";
    }
}
