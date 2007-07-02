#!/usr/bin/perl
##
## uribl.pl, 2006.05.16, SJ
##

use Net::DNS;

$dom = "spamuri.yourdomain.com"; # your rbl domain
@dnsip = ("127.0.0.1"); # IP-address of a resolver DNS server

$ttl = 1800;
$ip = "127.0.0.2";

$res = Net::DNS::Resolver->new(nameservers => [@dnsip]);

while(<STDIN>){
   chomp;
   $_ =~ s/URL\*//;
   $A{$_}++;
}

foreach $x (sort keys %A){
   print STDERR "processing $x ... ";

   if($x =~ /\d{1,3}\.\d{1,3}/){
      print "+$x.$dom:$ip:$ttl\n";
      print STDERR "ok\n";
      next;
   }

   @ns = &rezolv_ns($x);
   if(@ns){
      print "+$x.$dom:$ip:$ttl\n";
      print STDERR "ok\n";
   }
   else {
      print STDERR "BAD\n";
   }
}

sub rezolv_ns {
   my ($domain) = @_;
   my @r = ();

   my $qry = $res->query($domain, "NS");

   if($qry){
      foreach $rr (grep { $_->type eq 'NS' } $qry->answer){
         push(@r, $rr->nsdname);
      }
   }

   return @r;
}

