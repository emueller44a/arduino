use strict;
use warnings;

use IO::Socket;
use Time::HiRes qw(usleep time);


my $sock = IO::Socket::INET->new(
    Proto    => 'udp',
    PeerPort => 8888,
    PeerAddr => '192.168.2.1'
) or die "Could not create socket: $!\n";

my $start_time;
$start_time = time;

# https://www.arduino.cc/en/Reference/KeyboardModifiers
my $KEY_LEFT_CTRL = chr(0x80);
my $KEY_LEFT_ALT = chr(0x82);
my $KEY_DELETE = chr(0xD4);
my $KEY_RETURN = chr(0xB0);
my $KEY_TAB = chr(0xB3);
my $KEY_ESC = chr(0xB1);

#print "waiting 10 seconds...\n";
#sleep 10;

# windows logon

$sock->send("\x{02}\x{80}");
$sock->send(chr(0x02).$KEY_LEFT_CTRL);
$sock->send(chr(0x02).$KEY_LEFT_ALT);
$sock->send(chr(0x02).$KEY_DELETE);
#$sock->send(chr(0x02).$KEY_LEFT_CTRL);
#$sock->send(chr(0x02).$KEY_LEFT_ALT);
#$sock->send(chr(0x02).$KEY_DELETE);
sleep 0.1;
$sock->send(chr(0x03));


my $elapsed;
$elapsed = time - $start_time;
$elapsed = sprintf("%.2f", $elapsed);

print "elapsed: ".$elapsed."\n";


my $aaa = "

Hello

kkkkkkkkkkkkkHH

";
