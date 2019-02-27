#!/usr/bin/perl

# Minimal dhcpd server
# Probably not compliant with anything, but may be useful to discover the MAC addresses, and later to write the first dhcpd.conf

# very few lines have been stolen from eicn dhcpd server


# TODO : FRENCH -> ENGLISH !

# $Revision $
# $Author $
# $Date $
# $Header $
# $Id $
# $Log $
# $State: Exp $

use Socket;          # Socket module: for the IP connection
use strict;
use Sys::Hostname;


my $dhcpd_ip;


{
	my ($name,$aliases,$addrtype,$length,@addrs) = gethostbyname(hostname());
	$dhcpd_ip = $addrs[0];
	print "-",inet_ntoa($dhcpd_ip), "-\n";
}


my $config_file = "/etc/ka/ka.conf";
my %ka_config;

# read the configuration file ".$config_file."
open CONFIG, "< ".$config_file || die "Could not open ".$config_file;
while (<CONFIG>) {
                chomp;
                s/#.*//;                # remove comments
                s/^ +//;                # remove leading spaces
                s/ +$//;                # remove trailling spaces
                next unless length;
                my ($key, $val) = split / *= */;
                $ka_config{$key} = $val;
}
close CONFIG;

# get a config value. If this value is mandatory and does not exist, die.
sub get_ka_config($$)
{
        my ($key, $mandatory) = @_;

        if ($mandatory > 0) {
                die "Missing config key in ".$config_file." : ".$key unless exists $ka_config{$key};
        }
        return $ka_config{$key};
}


my $tftp_server = get_ka_config("tftp_server", 1);
my $gateway = get_ka_config("gateway", 1);
my $dns = get_ka_config("dns", 1);
my $domain = get_ka_config("domain", 1);
my $netmask = get_ka_config("netmask", 1);
my $pxefilename = get_ka_config("pxe_filename", 1);
my $option135 = get_ka_config("option135", 1);
my $subnet = get_ka_config("subnet", 1);


my $SERVER_PORT = 67;
my $CLIENT_PORT = 68;
my $MAGIC_COOKIE="\x63\x82\x53\x63";

my $frame;

my @messages_types = ("UNUSED", "DHCPDISCOVER", "DHCPOFFER", "DHCPREQUEST", "DHCPDECLINE", "DHCPACK", "DHCPNACK", "DHCPRELEASE", "DHCPINFORM");
my %transactions;

## HASH that gives the correspondance mac addr --> ip
my %ips;

## HASH that gives the correspondance ip --> mac addr
my %hardwareaddr;

## array with all the IP I know, just to keep them in the same order as in the data file
my @ips_list;


## Read the data file, and fill %ips, %hardwareaddr and @ips_list
sub read_datafile()
{
	my ($filename) = @_;
	
	open (DATAF, "< ". $filename) || die "could not open ".$filename;
	while (<DATAF>)
	{
		chomp;
		my ($ip, $hardware) = split(/ /);
		#printf "trouve une ligne avec ip = -%s- et hard = -%s-\n", $ip, $hardware;
		if ($hardware ne "")
		{
			$ips{$hardware}=$ip;
			$hardwareaddr{$ip}=$hardware;
		}
		else
		{
			$hardwareaddr{$ip}="free";
		}
		
		push @ips_list, $ip;
		
		printf "Read IP : %s - %s\n", $ip, $hardware;
	}
	close DATAF;
}
	
sub write_datafile()
{
	my ($filename) = @_;
	my $ip;
	open (DATAF, "> ". $filename) || die "could not open ".$filename;
	foreach $ip (@ips_list) # pour chaque ip faire :
	{
		printf DATAF "%s %s\n", $ip, $hardwareaddr{$ip}
	}
	close DATAF;
}

## find an IP for a given mac adress
## returns "none" if none is free
sub get_ip_for()
{
	my ($hard) = @_;
	if ($ips{$hard} ne "")
	{
		return $ips{$hard};
	}
	else
	{
		my $ip;
		# on cherche une ip libre
		foreach $ip (@ips_list) 
		{
			if ($hardwareaddr{$ip} eq "free")
			{
				$ips{$hard} = $ip;
				$hardwareaddr{$ip} = $hard;
				return $ip;
			}
		}
		return "none";
	}
}
		

# debug function
sub print_transactions()
{
	foreach $_ (keys(%transactions))
	{
		printf (" X %s M %s S %s\n", $_, $transactions{$_}{mac}, $transactions{$_}{state});
	}
}

## process an option found in a DHCP frame
sub traite_option()
{
	my ($xid, $tag, $len, $data) = @_;
	
	if ($tag == 53) # DHCP Message type
	{
		my $type = unpack("C", $data);
		
		printf(" - Message type : %s", @messages_types[$type]);
    	        $transactions{$xid}{state} = $type;
	}
	if ($tag == 55) # parameter request list
	{
		delete $transactions{$xid}{options}; # on vide la liste
		map { 
			my $op=$_; 
			#printf("Le client veut l'option %d\n", $op); 
			$transactions{$xid}{options}{$op}="yes"; 
		} unpack("C*", $data);
		
	}
	if ($tag == 60) # Vendor
	{
#		printf("Vendor : %s\n", $data);
	}
	if ($tag ==57) # Maximum message size
	{
#		printf("Max message size : %d\n", unpack("n", $data));
	}	
}


## parse a block of options (the end of a frame)
sub parse_options()
{
	my ($xid, $data) = @_;
	###my @data = unpack("C", $data);
	my $ptag;
	my $tag;
	my $len;
	my $notfinished = 1;
	my $lendata;
	$lendata = length($data);
#	printf("options length = %d\n", $lendata);
	
	for ($ptag = 0;($notfinished && ($ptag < $lendata));)
	{
		$tag = unpack("C", substr($data, $ptag, 1));
		if (($tag != 0) && ($tag !=255))
		{
			$len = unpack("C", substr($data, $ptag+1, 1));
			#printf("ptag = %d,  Option %d, len %d\n", $ptag, $tag, $len);
			&traite_option($xid, $tag, $len, substr($data, $ptag + 2, $len));
			$ptag += $len + 2;			
		}
		else
		{
			if ($tag == 255)
			{
				$notfinished = 0;
			}
			else
			{
#				printf("ptag = %d, Option zero\n", $ptag);
			}
			$ptag++;
		}
	}
	
}

## returns a string corresponding to an option we want to add
## does not really 'add'
sub add_option()
{
	my ($tag, $data) = @_;
	
	my $len = length($data);
	
	my $option = pack("C", $tag);
	$option .= pack("C", $len);
	$option .= $data;
	
	return $option;
}

## the same, but returns an empty string if the client did not tell explicitely that he accepts this option
sub add_option_if()
{
	my ($xid, $tag, $data) = @_;
	
	if ($transactions{$xid}{options}{$tag} eq "yes")
	{
		#printf "Ajout de l'option numero %d\n", $tag;
		return &add_option($tag, $data);
	}
	else
	{
		return "";
	}
}

	

sub send_reply()
{
	my ($xid, $yiaddr, $siaddr, $file, $options) = @_;

	substr($frame, 0, 1) = pack("C", 2); # bootREPLY	
	substr($frame, 4, 4) = pack("H*",$xid);
	substr($frame, 16, 4) = $yiaddr;
	substr($frame, 20, 4) = $siaddr;
	my $i; my $c=""; for ($i=0; $i <128; $i++) { $c .="\x00";}
	substr($frame, 108 ) = pack("a*", $file) . $c;
	substr($frame, 236, 4) = $MAGIC_COOKIE; 
	substr($frame, 240) = $options;
	printf " - Sending reply with IP=%s\n", inet_ntoa($yiaddr);
	
	send (SOCK, $frame, 0, sockaddr_in($CLIENT_PORT, inet_aton("255.255.255.255")) )|| warn "send error: $!";
	
}





printf "CONFIG :
tftp_server = %s
gw = %s
dns = %s
domain = %s
netmask = %s
pxefilename = %s
option 135 = %s\n", $tftp_server, $gateway, $dns, $domain, $netmask, $pxefilename, $option135;




&read_datafile("data");




    

my($paddr_c,$recv_time,$port_c,$iaddr_c);
my($op,$htype,$hlen,$cookie,$bootp);
my $proto=getprotobyname("udp"); # select the transfert protocol

# create socket:
socket(SOCK,PF_INET,SOCK_DGRAM,$proto) || warn "socket error: $!";

# set socket options 
setsockopt(SOCK, SOL_SOCKET, SO_REUSEADDR,1) || warn "setsockopt error: $!";
setsockopt(SOCK, SOL_SOCKET, SO_BROADCAST,1) || warn "setsockopt error: $!";

# bind socket 
bind(SOCK,sockaddr_in($SERVER_PORT, INADDR_ANY)) || warn "bind error: $!";




while(1){
	my $options;
	my $hardware;
	my $time;
	my $xid;
	my $cur_ip;
	$paddr_c=recv(SOCK,$frame,1500,0);    # receive frame from socket
	$recv_time=time;                      # time in seconde from 1/1/1970
	($port_c,$iaddr_c)=sockaddr_in($paddr_c);
	print "\n";

	#    printf("Source : %s:%d\n", inet_ntoa($iaddr_c), $port_c);

	next if (length($frame) < 240); # want a complete frame

	$op=substr($frame, 0, 1) ;            # get the op filed (BOOTREQUEST OR BOOTREPLY)
	$htype=substr($frame, 1, 1);
	$hlen=substr($frame, 2, 1);
	$xid=substr($frame, 4, 4);
	$cookie=substr($frame, 236, 4);
	$bootp=substr($frame, 240, 1);
	$options=substr($frame, 240);


	($hardware)=($frame=~ /^.{28}(......)/s);    
	$hardware = unpack("H*",$hardware);
	$xid = unpack("H*", $xid);
	print "Got a frame : ";    
	print "MAC Address : ", $hardware;

	#    printf ("XID = %s\n", $xid);


	# just for testing #
	next if ($hardware ne "000102043b23"); # n'emmerdons pas tout le monde



	next if($op eq "\x02");               # ignore if it is a BOOTREPLY, as we are a server

	print " - Not a BOOTREPLY";

	next if($cookie ne $MAGIC_COOKIE);    # need the magic cookie

	print " - MAGIC_COOKIE is ok";

	if($bootp ne "\xff")
	{
	#print " - Not a BOOTP frame\n";
	&parse_options($xid, $options);

	$transactions{$xid}{mac} = $hardware;

	($time)=(localtime()=~ /^\S{3}\s(\S+\s+\S+\s\S+)/); # local time

	#	    &print_transactions();
	}

	my $options = "\x00"; # option zero

	$cur_ip = &get_ip_for($hardware);
	#    printf "cur_ip = %s\n", $cur_ip;

	next if ($cur_ip eq "none"); # pas d'ip libre

	if ($transactions{$xid}{state} == 1)
	{	
		$options .= &add_option(53, pack("C", 2));
	} # DHCPOFFER
	else
	{	
	# DHCPACK
		$options .= &add_option(53, pack("C", 5));
		&write_datafile("data");
	} 

	$options .= &add_option_if($xid, 54, $dhcpd_ip); # SERVER ID		
	#    $options .= &add_option_if($xid, 51, pack("N", 5000000)); # LEASE TIME

	$options .= &add_option_if($xid, 1, inet_aton($netmask)); # masque de sous reseau
	$options .= &add_option_if($xid, 3, inet_aton($gateway)); # routeur
	$options .= &add_option_if($xid, 6, inet_aton($dns)); # DNS
	#    $options .= &add_option_if($xid, 12, "caiapo2" ); # hostname
	$options .= &add_option_if($xid, 15, $domain ); # domain
	#    $options .= &add_option_if($xid, 17, "icioula" ); # domain    
	#    $options .= &add_option_if($xid, 40, "equi2par" ); # domain    



	$options .= &add_option(135, sprintf($option135, $cur_ip)); # script bpbatch
	$options .= "\xff"; # option 255
	&send_reply($xid, inet_aton($cur_ip), inet_aton($tftp_server), $pxefilename, $options);

}
