// SPDX-License-Identifier: GPL-2.0-only

/* TODO: mem op codes and direct access */

#include <linux/io.h>
#include <linux/iopoll.h>
#include <linux/of_address.h>
#include <linux/platform_device.h>
#include <linux/spi/spi-mem.h>

#define CC_SPI_CTRL 0x00
#define CC_SPI_ADDR 0x04
#define CC_SPI_DATA 0x08

#define CC_SPI_OPCODE   0x000000ff
#define CC_SPI_1B_RD    0x00000100
#define CC_SPI_3B_ADDR  0x00000200
#define CC_SPI_4B_DATA  0x00000400
#define CC_SPI_CS       0x00001000
#define CC_SPI_4B_ADDR  0x00006000 //What does each bit do?
#define CC_SPI_ACTIVE   0x80000000

#define WINDOW_MEM_SZ (16 << 20)

struct bcm_cc_spi_access {
	void __iomem *regs;
	void __iomem *dirwin;
	//struct resource *dirwin;
};

static void bcm_cc_spi_set_cs(struct spi_device *spi, bool active)
{
	struct bcm_cc_spi_access *cc_spi = spi_controller_get_devdata(spi->controller);

	/* CS will be set when the transfer starts */
	if (!active)
		writel(0, cc_spi->regs + CC_SPI_CTRL);
}

static int bcm_cc_spi_exec_op(struct spi_device *spi, u32 opval)
{
	struct bcm_cc_spi_access *cc_spi = spi_controller_get_devdata(spi->controller);
	u32 ctrl_val;
	int ret = -ETIMEDOUT;

	ctrl_val =  opval | CC_SPI_CS;
	writel(ctrl_val | CC_SPI_ACTIVE, cc_spi->regs + CC_SPI_CTRL);

	ret = read_poll_timeout(readl, ctrl_val, !(ctrl_val & CC_SPI_ACTIVE),
			0, 500, false, cc_spi->regs + CC_SPI_CTRL);

	return ret;
}



static int bcm_cc_spi_xfer_one(struct spi_controller *ctrl, struct spi_device *spi,
		struct spi_transfer *xfer)
{
	struct bcm_cc_spi_access *cc_spi = spi_controller_get_devdata(spi->controller);
	u8 *rx_buf;
	const u8 *tx_buf;
	u32 opval = 0;
	int cnt;
	int ret = 0;

	tx_buf = xfer->tx_buf;
	rx_buf = xfer->rx_buf;
	cnt = xfer->len;

	while (cnt) {
		if (tx_buf) {
			opval = *tx_buf;
			tx_buf++;
		}
		ret = bcm_cc_spi_exec_op(spi, opval);
		if (ret)
			break;

		if (rx_buf) {
			*rx_buf = temp & 0xFF;
			rx_buf++;
		}
		cnt--;
	}

	spi_finalize_current_transfer(ctrl);

	return ret;
}

static int bcm_cc_spi_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct spi_controller *host;
	struct bcm_cc_spi_access *cc_spi;
	int ret;

	host = devm_spi_alloc_host(dev, sizeof(*cc_spi));
	if (!host)
		return -ENOMEM;

	platform_set_drvdata(pdev, host);
	cc_spi = spi_controller_get_devdata(host);

	cc_spi->regs = devm_platform_get_and_ioremap_resource(pdev, 0, NULL);
	if (IS_ERR(cc_spi->regs))
		return PTR_ERR(cc_spi->regs);

	host->dev.of_node = dev->of_node;
	host->bus_num = -1;
	host->set_cs = &bcm_cc_spi_set_cs;
	host->transfer_one = bcm_cc_spi_xfer_one;
	host->num_chipselect = 1;

	ret = spi_register_controller(host);
	if (ret) {
		dev_err(&pdev->dev, "Could not register SPI controller: %d\n", ret);
		return -ENODEV;
	}

	return 0;
}

static const struct of_device_id bcm_cc_spi_match[] = {
	{ .compatible = "brcm,bcma-cc-spi", },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, bcm_cc_spi_match);

static struct platform_driver bcm_cc_spi_driver = {
	.driver = {
		.name = "BCM CC SPI",
		.of_match_table = bcm_cc_spi_match,
	},
	.probe = bcm_cc_spi_probe,
};
module_platform_driver(bcm_cc_spi_driver);
