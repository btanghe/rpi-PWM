rpi-PWM
=======

linux pwm driver using raspberry pi pin 18 - expansion header 12

This is a raspberry pi linux device driver using the linux pwm subsystem
(I've tested on linux kernel 3.11, 3.13 and 3.14)

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, 
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES 
OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR 
OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR 
THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Installation steps

1 - modify arch/arm/mach-bcm2708/bcm2708.c
this is on the linux 3.14 source

line 222:

	static struct resource bcm2835_pwm_resources[] = {
		{
		 .start = BCM2708_PERI_BASE,
		 .end = BCM2708_PERI_BASE + SZ_4K - 1,
		 .flags = IORESOURCE_MEM,
		 }
	};

	static struct platform_device pwm_bcm2835_device = {
		.name = "pwm-bcm2835",
		.id = 1,
		.resource = bcm2835_pwm_resources,
		.num_resources = ARRAY_SIZE(bcm2835_pwm_resources),
	};

line 864:

        bcm_register_device(&pwm_bcm2835_device);


2 - add the pwm source to the pwm directory
	
	git am < pwm-bcm2835.patch

3 - select the kernel driver
	
	make ARCH=arm menuconfig
		> Device Drivers > [*] Pulse-Width Modulation (PWM) Support
		> Device Drivers > [*] Pulse-Width Modulation (PWM) Support > <*> BCM2835 PWM support 

4 - recompile everything

Using the driver

## export the pwm
	echo 0 > /sys/class/pwm/pwmchip0/export			
	cd /sys/class/pwm/pwmchip0/pwm0/

## set the duty cycle and period (minimum period is 1us)

	echo 100000 > duty_cycle	//duty cycle is 100us
	echo 30000  > period		//period of 30 us
	echo 1 > enable

## change polarity

	echo 0 > enable			//disable the pwm channel
	echo inversed > polarity
	echo 1 > enable			//enable the pwm channel

## back to normal

	echo 0 > enable
	echo normal > polarity
	echo 1 > enable

Some additional information

Because the memory mapped io registers of the bcm2835 pwm hardware are spreaded 
over the memory mapped io, 
gpio config 0x20200004 - clk config 0x201010A0 - pwm configuration 0x2020C000
I've used the base address of the memory mapped io 
so I can use positive offsets

this give some nasty stuff in for instance, /proc/iomem
