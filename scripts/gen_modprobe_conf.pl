$kinds2all_modules = {
                       'usb' => [
'usb-uhci', 'usb-ohci', 'ehci-hcd'
                                ],

                       'network' => [
'atl1', 'atl1c', 'atl1e', 'be2net', 'bnx2', 'bnx2x', 'cxgb', 'cxgb3', 'dl2k', 'e1000', 'e1000e', 'et131x', 'igb', 'ipg', 'ixgb', 'ixgbe', 'myri_sbus', 'netxen_nic', 'ns83820', 'qla3xxx', 'r8169', 's2io', 'sfc', 'sxg_nic', 'sis190', 'sk98lin', 'skge', 'sky2', 'slicoss', 'spidernet', 'tehuti', 'tg3', 'via-velocity', 'virtio_net', 'vxge', 'yellowfin', 'bcm5820', 'bcm5700', '3c501', '3c503', '3c505', '3c507', '3c509', '3c515', '3c990', '3c990fx', '82596', 'ac3200', 'acenic', 'aironet4500_card', 'amd8111e', 'at1700', 'atl2', 'atp', 'bcm4400', 'cassini', 'cs89x0', 'de600', 'de620', 'depca', 'dmfe', 'e2100', 'eepro', 'eexpress', 'enic', 'eth16i', 'ewrk3', 'hp', 'hp-plus', 'hp100', 'iph5526', 'jme', 'lance', 'ne', 'ni5010', 'ni52', 'ni65', 'nvnet', 'prism2_plx', 'qlge', 'r6040', 'rcpci', 'rhineget', 'sb1000', 'sc92031', 'smc-ultra', 'smc9194', 'smsc9420', 'smsc95xx', 'tc35815', 'tlan', 'uli526x', 'vmxnet3', 'b44', 'com20020-pci', 'de2104x', 'defxx', 'dgrs', 'e100', 'eepro100', 'epic100', 'fealnx', 'hamachi', 'natsemi', 'ne2k-pci', 'pcnet32', 'plip', 'sis900', 'skfp', 'starfire', 'tulip', 'typhoon', 'via-rhine', 'winbond-840', 'forcedeth', 'sungem', 'sunhme', '3c59x', '8139too', '8139cp', 'cpmac', 'niu', 'sundance',

			'ide' => [
'aec62xx', 'ali14xx', 'alim15x3', 'amd74xx', 'atiixp', 'cmd64x', 'cy82c693', 'cs5520', 'cs5530', 'cs5535', 'cs5536', 'delkin_cb', 'dtc2278', 'hpt34x', 'hpt366', 'ns87415', 'ht6560b', 'it8172', 'it8213', 'it821x', 'jmicron', 'opti621', 'pdc202xx_new', 'pdc202xx_old', 'piix', 'qd65xx', 'rz1000', 'sc1200', 'serverworks', 'siimage', 'sis5513', 'slc90e66', 'tc86c001', 'triflex', 'trm290', 'tx4938ide', 'tx4939ide', 'umc8672', 'via82cxxx', 'ide-pci-generic', 'ide-generic',
			],

		'sata' => [
'ahci', 'aic94xx', 'ata_adma', 'ata_piix', 'pata_pdc2027x', 'pdc_adma', 'sata_fsl', 'sata_inic162x', 'sata_mv', 'sata_nv', 'sata_promise', 'sata_qstor', 'sata_sil', 'sata_sil24', 'sata_sis', 'sata_svw', 'sata_sx4', 'sata_uli', 'sata_via', 'sata_vsc', 'sx8', 'ata_generic', 'mv-ahci', 'pata_ali', 'pata_amd', 'pata_artop', 'pata_atiixp', 'pata_atp867x', 'pata_bf54x', 'pata_cmd640', 'pata_cmd64x', 'pata_cs5520', 'pata_cs5530', 'pata_cs5535', 'pata_cs5536', 'pata_cypress', 'pata_efar', 'pata_hpt366', 'pata_hpt37x', 'pata_hpt3x2n', 'pata_hpt3x3', 'pata_isapnp', 'pata_it8172', 'pata_it8213', 'pata_it821x', 'pata_jmicron', 'pata_legacy', 'pata_marvell', 'pata_mpiix', 'pata_netcell', 'pata_ninja32', 'pata_ns87410', 'pata_ns87415', 'pata_oldpiix', 'pata_opti', 'pata_optidma', 'pata_pdc2027x', 'pata_pdc202xx_old', 'pata_piccolo', 'pata_platform', 'pata_qdi', 'pata_radisys', 'pata_rdc', 'pata_rz1000', 'pata_sc1200', 'pata_sch', 'pata_serverworks', 'pata_sil680', 'pata_sis', 'pata_sl82c105', 'pata_triflex', 'pata_via', 'pata_winbond', 'pata_acpi',
],

                       'scsi' => [
'aha152x_cs', 'fdomain_cs', 'nsp_cs', 'qlogic_cs', 'ide-cs', 'pata_pcmcia', 'sym53c500_cs', '53c7,8xx', 'a100u2w', 'advansys', 'aha152x', 'aha1542', 'aha1740', 'AM53C974', 'atp870u', 'be2iscsi', 'bfa', 'BusLogic', 'dc395x', 'dc395x_trm', 'dmx3191d', 'dtc', 'eata', 'eata_dma', 'eata_pio', 'fdomain', 'g_NCR5380', 'in2000', 'initio', 'mpt2sas', 'mvsas', 'NCR53c406a', 'nsp32', 'pas16', 'pci2220i', 'pm8001', 'psi240i', 'qla1280', 'qla2x00', 'qla2xxx', 'qlogicfas', 'qlogicfc', 'seagate', 'shasta', 'sim710', 'stex', 'sym53c416', 't128', 'tmscsim', 'u14-34f', 'ultrastor', 'vmw_pvscsi', 'wd7000', 'aic7xxx', 'aic7xxx_old', 'aic79xx', 'pci2000', 'qlogicfas408', 'sym53c8xx', 'lpfc', 'lpfcdd', 'ide-gd_mod', 'sd_mod', '3w-9xxx', '3w-sas', '3w-xxxx', 'a320raid', 'aacraid', 'arcmsr', 'cciss', 'cpqarray', 'cpqfc', 'DAC960', 'dpt_i2o', 'gdth', 'hpsa', 'hptiop', 'i2o_block', 'imm', 'ipr', 'ips', 'it8212', 'it821x', 'iteraid', 'megaide', 'megaraid', 'megaraid_mbox', 'megaraid_sas', 'mptfc', 'mptsas', 'mptscsih', 'mptspi', 'pdc-ultra', 'pmcraid', 'ppa', 'qla2100', 'qla2200', 'qla2300', 'qla2322', 'qla4xxx', 'qla6312', 'qla6322',
                                 ]
                     };


my @l = map { /^(\S+)\s*:/ ? $1 : () } `lspcidrake`;

my %kinds2modules = map { 
    $_ => [ intersection(\@l, $kinds2all_modules->{$_}) ];
} qw(usb ide sata scsi);

$kinds2modules{network} = [
  grep {
	  my $l = $_;
	  scalar grep { $_ eq $l } @{ $kinds2all_modules->{network} }
  } @l
];

if (my @ide = @{$kinds2modules{ide}}) {
	    print "install ide-controller /sbin/modprobe ", join("; /sbin/modprobe ", @ide), "; /bin/true\n";
}

if (my @scsi = @{$kinds2modules{scsi}}) {
    print "install scsi_hostadapter /sbin/modprobe ", join("; /sbin/modprobe ", @scsi), "; /bin/true\n";
}

if (my @sata = @{$kinds2modules{sata}}) {
    print "install scsi_hostadapter /sbin/modprobe ", join("; /sbin/modprobe ", @sata), "; /bin/true\n";
}

if (my @usb = @{$kinds2modules{usb}}) {
    print "install usb-interface ", join(" ", @usb), "; /bin/true\n";
}

my $eth = 0;
foreach (@{$kinds2modules{network}}) {
    print "alias eth$eth $_\n";
    $eth++;
}

sub intersection { my (%l, @m); @l{@{shift @_}} = (); foreach (@_) { @m = grep { exists $l{$_} } @$_; %l = (); @l{@m} = () } keys %l }
