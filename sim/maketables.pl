#!/usr/bin/perl

sub read_isn {
    my $filename = $_[0];

    open(IF, $filename) || die("could not open $filename");
    my @isn_lines = <IF>;
    close(IF);

    %isns = ();

    foreach my $line (@isn_lines) {
        chop($line);
        if ($line =~ /^#/ or length($line) eq 0) {
            next;
        }

        my ($isn,$type,$regs,$opcode,$opcode2) = split(/\t/,$line);

        #print "isn $isn, type $type, regs $regs, opcode $opcode, opcode2 $opcode2\n";

        %isns->{$isn} = { 'isn' => $isn, 'type' => $type, 'regs' => $regs, 'opcode' => $opcode, 'opcode2' => $opcode2 };
    }

    # sort the list by id and put in array
    @isns =
        sort {$isns{$a}{opcode} <=> $isns{$b}{opcode}}
        keys %isns;
}

sub dump_isn_enum {
    print "enum {\n";

    foreach my $i (@isns) {

	print "    ISN_".%isns->{$i}{isn}.",\n";
    }

    print "};\n";
}

sub dump_isn_table {
    print "raw_isn_t raw_isns[] = {\n";

    foreach my $i (@isns) {
	my $r = %isns->{$i}{regs};
	$r = "NONE" if ($r eq '-');
	$r = "R_".$r;

	my $op2 = %isns->{$i}{opcode2};

	print "    { ISN_".%isns->{$i}{isn}.", \"".%isns->{$i}{isn}."\", I_".%isns->{$i}{type}.", $r, 0".%isns->{$i}{opcode}.", 0".$op2." },\n";
    }

    print "    { -1, (char *)0, 0, 0, 0, 0 }\n";

    print "};\n";
}

$isn_filename = "isn.txt";

read_isn($isn_filename);
dump_isn_enum;
dump_isn_table;
