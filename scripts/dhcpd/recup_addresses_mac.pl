
# while booting the machines, do tail -f /var/log/messages > foo
# then recup_addresses_mac.pl < foo and cut'n paste the result to dhcpd.conf
# !! this script needs heavy tuning (IP, host names, etc.)
# note that we assume that the node machine_num has ip xxx.xxx.xxx.num (not really, hostnames in dhcpd.conf are never used)

# $Revision: 1.1 $
# $Author: sderr $
# $Date: 2002/05/23 10:23:05 $
# $Header: /cvsroot/ka-tools/ka-deploy/scripts/dhcpd/recup_addresses_mac.pl,v 1.1 2002/05/23 10:23:05 sderr Exp $
# $Id: recup_addresses_mac.pl,v 1.1 2002/05/23 10:23:05 sderr Exp $
# $Log: recup_addresses_mac.pl,v $
# Revision 1.1  2002/05/23 10:23:05  sderr
# removed bpbatch
#
# Revision 1.2  2001/05/03 12:34:41  sderr
# Added CVS Keywords to most files. Mostly useless.
#
# $State: Exp $


my $mac;
my $ip;
my $already;


# open dhcpd.conf to avoid duplicates
open F, "< /etc/dhcpd.conf";
while (<F>){
	chop $_;
	if (/129.88.96./)
	{
		$_ =~ s/.*(129.88.96.[0-9]*).*/$1/;
		$already{$_} = "yes";
	}
}
close F;


while (<>)
{
	if (/DHCPOFFER/)
	{
		chomp $_;
		$cemac = $ceip = $_;
		$cemac =~ s/.*to ([^ ]*) via.*/$1/;
		$ceip =~ s/.*on ([^ ]*) to.*/$1/;
		$mac{$ceip} = $cemac;
		$ip{$cemac} = $ceip;
	}
}
foreach $ceip (sort(keys(%mac))) {
	#printf "%s <==> %s\n", $ceip, $mac{$ceip};        
	$num=$ceip;
	if ($already{$ceip} ne "yes")
	{
	($ga, $bu, $zo, $num) = split(/\./, $ceip);
	printf "
	host icluster%s 
        {
                hardware ethernet %s;
                fixed-address %s;
                filename \"ka/pxelinux.0\";
                next-server 129.88.96.252;
        }
"
	, $num, $mac{$ceip}, $ceip, $num;        
	}
}
