========
Chris,

This one gave me some trouble too.  I finally discovered that you need to
ACK the receipt of the protocol array with the protocol array byte as the
data for the ACK command.


Example:

nDummyData = Pid_Protocol_Array;
_PutGpsMessage(Pid_Ack_Byte,(BYTE*)&nDummyData,2);


I have fully tested ActiveX and C++ library for the Garmin.  You can find a
fully functioning demo and more information at www.stempsoft.com.


--
Bruce E. Stemplewski
www.stempsoft.com


"Chris Matthee" <cmatthee@csir.co.za> wrote in message
news:1022842049.386905@drp.csir.co.za...
> Hi
>
> I have a eTrex Venture and I'm trying to talk to the unit using the Garmin
> protocol.  According to the protocol, if one sends a Request Product Data
> command, the unit will respond with the product data (all units support
> this).  If the unit support the Capabilities Protocol, then the unit will
> automatically send this after the product data.  If not, then one must
look
> the unit capability up in the table.
>
> I'm sending the request command and I'm getting the product data from the
> unit, but nothing after that.  This should mean that the unit does not
> support the Capabilities Protocol and that I should use the table.  But,
the
> product id of the eTrex is not in the supplies table.
>
> Does anyone know where I can find more detailed information about the
eTrex
> implementation of the Garmin protocol?  Or am I doing something wrong and
> the eTrex should send the capabilities list and I'm not receiving it?
>
> Thanks
> Chris
>
>
>
>



========
Bruce E. Stemplewski wrote:
> Chris,
> 
> This one gave me some trouble too.  I finally discovered that you need to
> ACK the receipt of the protocol array with the protocol array byte as the
> data for the ACK command.
> 

(It might not be obvious, but) You need to ACK EVERY packet that you 
recieve (APart from ack packets themselves of course) in order for the 
next packet to be sent.

H


-- 

I don't suffer from Insanity... 	| Linux User #16396
	I enjoy every minute of it...	|
					|
http://www.travellingkiwi.com/ 
	|


========
You are correct but the protocol array is a special one.   The previous
packet must be acked with the product data byte.  Other packets aren't this
picky.

"Hamish Marson" <hamish@travellingkiwi.com.nospam> wrote in message
news:3D00955A.5040407@travellingkiwi.com.nospam...
> Bruce E. Stemplewski wrote:
> > Chris,
> >
> > This one gave me some trouble too.  I finally discovered that you need
to
> > ACK the receipt of the protocol array with the protocol array byte as
the
> > data for the ACK command.
> >
>
> (It might not be obvious, but) You need to ACK EVERY packet that you
> recieve (APart from ack packets themselves of course) in order for the
> next packet to be sent.
>
> H
>
>
> --
>
> I don't suffer from Insanity... | Linux User #16396
> I enjoy every minute of it... |
> |
> http://www.travellingkiwi.com/
> |
>



========
To my mind it is a silly bit of protocol in that you don't know whether you
will get the protocol array or not. You just have to wait and see. Not very
deterministic!

Mike.
--
If reply address = connectfee, add an r because it is free not fee.
Bruce E. Stemplewski <Bruce.Stemplewski@nospamstempsoft.com> wrote in
message news:ZpcM8.1840$oC3.1839924@news2.news.adelphia.net...
> You are correct but the protocol array is a special one.   The previous
> packet must be acked with the product data byte.  Other packets aren't
this
> picky.




========
Bruce E. Stemplewski wrote:
> You are correct but the protocol array is a special one.   The previous
> packet must be acked with the product data byte.  Other packets aren't this
> picky.

You mean the packetid field? That has to contain (According to Garmins 
specs anyway :) the packetid of the packet being acked. I wasn't aware 
you could get away with not poviding it for ANY of the recieved packets...


H


-- 

I don't suffer from Insanity... 	| Linux User #16396
	I enjoy every minute of it...	|
					|
http://www.travellingkiwi.com/ 
	|


========
Ok, I found the problem.  My own stupidity.

The packet containing the capability array also contains a character 3 (in
my specific situation).  My software was seeing this as a end of packet
character and then the AnalysePacket procedure returned with an error
because the packet failed the syntax checking (incorrect length, incorrect
terminating characters).

Made some changes and now the packet is recognized as legal.

So, the eTrex does indeed comply to A001.

Chris




========
Is there a more up to date document describing the garmin interface
available somewhere?  The pdf file on the garmin web site does not cover
A900 series of protocols and my gps reports supporting three of them.  Where
can I find more information about this?

Thanks
Chris




