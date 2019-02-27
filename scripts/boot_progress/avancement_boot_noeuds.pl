#! /usr/bin/perl
# this script watches /var/log/messages to know in which state are the nodes, and displays their status : rebooting, starting Linux, etc
# needs /etc/hosts to be OK for the NFS part to work

# TODO : FRENCH -> ENGLISH !

# $Revision: 1.3 $
# $Author: sderr $
# $Date: 2001/06/20 12:36:14 $
# $Header: /cvsroot/ka-tools/ka-deploy/scripts/boot_progress/avancement_boot_noeuds.pl,v 1.3 2001/06/20 12:36:14 sderr Exp $
# $Id: avancement_boot_noeuds.pl,v 1.3 2001/06/20 12:36:14 sderr Exp $
# $Log: avancement_boot_noeuds.pl,v $
# Revision 1.3  2001/06/20 12:36:14  sderr
# Major update of avancement_boot : now does not need tail anymore, thus the 'go' script has been removed
#
# Revision 1.2  2001/05/03 12:34:41  sderr
# Added CVS Keywords to most files. Mostly useless.
#
# $State: Exp $

use Gtk;         # load the Gtk-Perl module
use strict;      # a good idea for all non-trivial Perl scripts

init Gtk;        # initialize Gtk-Perl
set_locale Gtk;  # internationalize (AHAHAHA Good Joke)

my $lignes=34;
my $colonnes=7;
my $noeud_max=225;
my $window = new Gtk::Window( "toplevel" );
my $table = new Gtk::Table( $lignes, $colonnes, 1 ); 

my %labels;
my %etats;
my %steps;

my %etats_str;
my %historique;
my %fenetre_historique;
my %label_historique;

$etats_str{"Linux1dem1"}="Linux1, debut";
$etats_str{"Linux1dem2"}="Linux1, IP ok";
$etats_str{"Linux1dem3"}="Linux1, OK";
$etats_str{"Linux2dem2"}="Linux2, IP ok";
$etats_str{"Linux2dem3"}="PRET";
$etats_str{"reboote"}="reboot...";
$etats_str{"Ghostdem"}="DOS demarre...";
$etats_str{"Ghost"}="GHOST";
$etats_str{"bpbatch"}="BpBatch (linux2?)";
$etats_str{"bootdhcpok"}="PXE, IP ok";
$etats_str{"ghostfini"}="Ghost fini, reboot";
$etats_str{"Linuxnfsroot"}="Linux-install";
$etats_str{"deplipok"}="Install, IP OK";
$etats_str{"Deploiement"}="Deploiement";
$etats_str{"kernel"}="Linux";
$etats_str{"hdboot"}="HdBoot";




sub update_text()
{
	my ($num) = @_;
	$labels{$num}->set_text($num . " " . $steps{$num}." " . $etats_str{$etats{$num}} );
}










sub traite_step
{
	my ($num, $cefichier) = @_;
	printf "traite step %s %s %s\n", $num, $cefichier, $etats{$num};

	open STEP, "< /tftpboot/".$cefichier;
	$_ = <STEP>;
	$steps{$num} = "?";
	if (/ready/)
	{
		if ($etats{$num} eq "bpbatch") {
			# donc on boote sur le HD (si script kda_deploy normal utilise, sinon on va tt de suite telecharger un noyau et ca va se voir)
			my $etat="hdboot";
			$etats{$num}=$etat;
		}
		$steps{$num} = "R";
	}
	if (/install/) { $steps{$num} = "I"; }
	if (/lilo/) { $steps{$num} = "L"; }	
	close STEP;
	&update_text($num);	
}


sub traite_tftp
{
	my ($num, $cefichier) = @_;
	my $etat = "";
	if ($cefichier eq "vmlinuz")
	{
		$etat="Linux1dem1";
	}
	if ($cefichier eq "ghostrecup.img")
	{
		$etat="Ghostdem";
	}
	if ($cefichier eq "bpbatch")
	{
		$etat="bpbatch";
		## en fait ca va surement devenir linux2dem1 (?)
	}
	$_=$cefichier;
	if (/linux/)
	{
		$etat="kernel";
	}
	if (/nfsroot/)
	{
		$etat="Linuxnfsroot";
	}
	if (/steps/)
	{
		# c'est le fichier d'etape
		&traite_step($num, $cefichier);
		return;
	}
	if ($etat ne "")
	{
		$etats{$num}=$etat;
		&update_text($num);	
	}
}

sub traite_dhcp
{
	my ($num) = @_;
	my $etat = $etats{$num};
	my $etat_res = "bootdhcpok";
	
	if ($etat eq "Linuxnfsroot") { $etat_res = "deplipok"; }
	if ($etat eq "kernel") { $etat_res = "Linux1dem2"; }
	if ($etat eq "hdboot") { $etat_res = "Linux2dem2"; }	
	
	$etats{$num}=$etat_res;
	&update_text($num);	

}

sub traite_dhcp_fin
{
	my ($num) = @_;
	my $etat_res = "ghostfini";
	$etats{$num}=$etat_res;
	&update_text($num);	

}


sub traite_nfs
{
	my ($num, $op, $rep) = @_;
	my $etat = $etats{$num};
	my $etat_res;
	if ($op eq "mount")
	{
		$etat_res = "mount (?)";
		if ($etat eq "Linux1dem2") { $etat_res = "Linux1dem3"; }	
		#if ($etat eq "Linux2dem2") { $etat_res = "Linux2dem3"; }		
		$etat_res = "Linux2dem3";
		$_=$rep;
		if (/NFSROOT/) { $etat_res="Deploiement"; }
	}
	else
	{
		$etat_res="reboote";
	}
	
	$etats{$num}=$etat_res;	
	&update_text($num);	

}

sub add_historique
{
	my ($num, $ligne)= @_;
	$historique{$num}.=$ligne;
	if ($fenetre_historique{$num} ne "non")
	{
		$label_historique{$num}->set_text("Messages pour le noeud ".$num.":\n".$historique{$num});
		return 1;
	}
}


# reste un pb, le timer n'est pas supprime quand on ferme la fenetre. <----- y'a plus !
#sub update_fenetre_historique
#{
#	my ($num, $label) = @_;
#	#printf "update Historique opur mosnieur %s\n", $num;	
#	#$label->set_text("Messages pour le noeud ".$num.":\n".$historique{$num});
#	return 1;
#}

sub ferme_historique
{
	my ($foo, $num) = @_;
	$fenetre_historique{$num}="non";
}

sub ouvre_fenetre_historique
{
	my ($foo, $num) = @_;
	
	if ($fenetre_historique{$num} ne "non")
	{
		# on a deja une fenetre, on ne fait que la montrer
		$fenetre_historique{$num}->show();
		return 1;
	}
	my $window = new Gtk::Window( "toplevel" );	
	my $label = new Gtk::Label();
	#printf "Historique opur mosnieur %s\n", $num;
	$label->set_justify( 'left' );	
	$label->set_text("Messages pour le noeud ".$num.":\n".$historique{$num});
	$window->add($label);
	$window->signal_connect( "delete_event", \&ferme_historique, ($num) );   	
	$label->show();
	$window->show();

	$fenetre_historique{$num}=$window;
	$label_historique{$num}=$label;
	
#	Gtk->timeout_add( 1000, \&update_fenetre_historique, ($num, $label) ); 
	return 1;
}




sub read_input
{
	($_)=@_; # je sais pas si ca a un sens ca...
	print $_;
	if (/DHCPACK/)
	{
		my $cemac;
		my $ceip;
		my $ligne = $_;
		
		chomp $_;
		$cemac = $ceip = $_;
		$cemac =~ s/.*to ([^ ]*) via.*/$1/;
		$ceip =~ s/.*on ([^ ]*) to.*/$1/;
		printf "DHCP : %s\n", $ceip;

		# on recupere le numero de machine
		my $num; 
		my $foo; 
		($foo, $foo, $foo, $num) = split(/\./, $ceip);
		traite_dhcp($num);		
		add_historique($num, $ligne);
	}
	if (/DHCPRELEASE/)
	{
		my $ceip;
		my $ligne = $_;
		
		chomp $_;
		$ceip = $_;
		$ceip =~ s/.*of ([^ ]*) from.*/$1/;
		printf "DHCP-Fin : %s\n", $ceip;

		# on recupere le numero de machine
		my $num; 
		my $foo; 
		($foo, $foo, $foo, $num) = split(/\./, $ceip);
		traite_dhcp_fin($num);		
		add_historique($num, $ligne);
	}
	if (/trying to get file/)
	{
		my $ceip;
		my $cefichier;
		my $ligne = $_;		

		chomp $_;
		$ceip = $cefichier = $_;
		$ceip =~ s/.*tftpd: ([^ ]*) try.*/$1/;	
		$cefichier =~ s/.*to get file: ([^ ]*) .*$/$1/;			
		printf "TFTP : %s --> %s\n", $cefichier, $ceip;
		
		# on recupere le numero de machine
		my $num; 
		my $foo; 
		($foo, $foo, $foo, $num) = split(/\./, $ceip);
		traite_tftp($num, $cefichier);
		add_historique($num, $ligne);		
	}
	# attention, celui-la recupere les mount ET unmount
	if (/mount request from icluster/)
	{
		my $ligne = $_;		
		chomp $_;

		my ($num,$op,$rep) = ($_, $_, $_);
		$num =~ s/.*from icluster([^:]*):.*$/$1/;
		$op =~ s/.*authenticated (.*) request from icluster.*$/$1/;
		$rep =~ s/.*\((.*)\).*$/$1/;
		printf "NFS : %s : %s %s\n", $num, $op, $rep;		
		traite_nfs($num, $op, $rep);		
		add_historique($num, $ligne);		
	}
}


# in theory there could be a small bug if the file does not end on a line boundary, but not important

my $file="/var/log/messages";
my $taille = 0;

sub check_input()
{
#printf("========> File GA <===========\n");
	while (<POUET>) { &read_input($_); }

	my @stats = stat $file;
	my $taillecur = $stats[7];
	if ($taillecur < $taille)
	{
		printf("========> File truncated <===========\n");
		close POUET;
		open POUET, "< ".$file;
	}
	$taille = $taillecur;
#printf("========> File BU <===========\n");
}




printf "%s\n", $file;
open POUET, "< ".$file;
seek POUET, 0, 2; # go to the end of the file







$window->border_width( 15 );
$window->add( $table );
$window->set_default_size( 1000, 500 ); 
# on cree la table
for (my $y=1; $y <= $lignes; $y++)
{
	for (my $x=0; $x < $colonnes; $x++)
	{
		my $num;
		my $celabel;
		my $cebouton;
		$num = $x * $lignes + $y;
		if ($num <= $noeud_max)
		{
			$celabel = new Gtk::Label( $num . " ? ");
			$cebouton = new Gtk::Button();
			$cebouton->add($celabel);
			$celabel->set_justify( 'left' );
			$labels{$num} = $celabel;
			$table->attach_defaults( $cebouton, $x, $x+1, $y-1, $y );
			$celabel->show();
			$cebouton->show();
			$cebouton->signal_connect( "clicked", \&ouvre_fenetre_historique, ($num) );
			$fenetre_historique{$num}="non";
		}
	}
}
$table->show();
$window->show();

#Gtk::Gdk->input_add(0, 'read', \&read_input, 0 );

Gtk::timeout_add(0, 1000, \&check_input);

main Gtk;
