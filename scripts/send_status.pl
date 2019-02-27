#!/usr/bin/perl -w
# redirect the execution to > /tmp/ka_log 2>&1 to get the exec output on golden node
use IO::Socket;

my $HOSTNAME = $ENV{HOSTNAME};

$server = IO::Socket::INET->new(
    LocalPort => 12345,
#    PeerAddr => '192.168.0.100',
    Type      => SOCK_STREAM,
    Reuse     => 1,
    Listen    => 5
    ) or die "Can't start the server\n";
print "Starting log server..\n";

while ($client = $server->accept()) {
    print $client "$HOSTNAME online\n";
    $pid = fork;
    die "Can't fork !" if ! defined ($pid);
    if ($pid ==0) {
	while ($data = <$client> ) {
	    print "<console> $data";
	    if ($data =~ /^exec\s/ ) { system($data) }
	}
    } else {
	open(FIFO, "tail -f /tmp/ka* |") or die $!;
	while ($data = <FIFO>) {
	    print $client $data;
	}
	close(FIFO);
    }
}

