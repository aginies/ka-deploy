#! /usr/bin/perl

# this script creates a dhcpd.conf from the data file of the mini dhcpd server

# $Revision: 1.2 $
# $Author: sderr $
# $Date: 2001/10/10 13:55:05 $
# $Header: /cvsroot/ka-tools/ka-deploy/scripts/dhcpd/create_dhcpd_conf.pl,v 1.2 2001/10/10 13:55:05 sderr Exp $
# $Id: create_dhcpd_conf.pl,v 1.2 2001/10/10 13:55:05 sderr Exp $
# $Log: create_dhcpd_conf.pl,v $
# Revision 1.2  2001/10/10 13:55:05  sderr
# Updates in documentation
#
# Revision 1.1  2001/06/22 13:19:47  sderr
# Added create_dhcpd_conf.pl, README, mkiplist, and the config.pm stuff
#
# $State: Exp $



## HASH that gives the correspondance mac addr --> ip
my %ips;

## HASH that gives the correspondance ip --> mac addr
my %hardwareaddr;

## array with all the IP I know, just to keep them in the same order as in the data file
my @ips_list;

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


## Read the data file, and fill %ips, %hardwareaddr and @ips_list
sub read_datafile()
{
	my ($filename) = @_;
	
	open (DATAF, "< ". $filename) || die "Could not open data file";
	while (<DATAF>)
	{
		chomp;
		my ($ip, $hardware) = split(/ /);

		$hardwareaddr{$ip}=$hardware;
		if ($hardware ne "")
		{
			$ips{$hardware}=$ip;
		}
		else
		{
			$ips{$hardware}="free";
		}
		
		push @ips_list, $ip;
	}
	close DATAF;
}
	
sub write_conf()
{
	
	printf "option subnet-mask %s;\noption domain-name \"%s\";\noption domain-name-servers %s;\noption routers %s;\n", $netmask, $domain, $dns, $gateway;

	printf "subnet %s netmask %s \n{\n\tgroup\n\t{\n\t\tdefault-lease-time -1;\n", $subnet, $netmask;

	
	
	
	
	my $ip;
	foreach $ip (@ips_list)
	{
		my $mac = $hardwareaddr{$ip};
		$mac =~ s/(..)(..)(..)(..)(..)(..)/$1:$2:$3:$4:$5:$6/;
		printf "
		host %s 
        	{
                	hardware ethernet %s;
                	fixed-address %s;
                	filename \"%s\";
                	next-server %s;
                	option option-135 \"%s\";
        	}
"
	, $ip, $mac, $ip, $pxefilename, $tftp_server, sprintf($option135, $ip);        

	}

	printf "\t}\n}\n";
}


&read_datafile("data");
&write_conf();
