$tftp_server="129.88.96.252";
$gateway="129.88.69.254";
$dns="129.88.30.1";
$domain="imag.fr";
$netmask="255.255.255.0";
$pxefilename="bpbajhtch";
$subnet="129.88.69.0"; # needed by ISC dhcpd

# option 135 is the name of the bpbatch script
# %s will be replaced by the ip of the client
# $option135 = "scripts/%s";
$option135 = "iclutest";
