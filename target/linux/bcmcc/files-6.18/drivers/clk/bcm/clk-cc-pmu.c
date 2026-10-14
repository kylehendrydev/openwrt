// SPDX-License-Identifier: GPL-2.0+

/* This definitely does not work in it's current state */

#include <linux/clk.h>
#include <linux/clk-provider.h>
#include <linux/platform_device.h>
#include <linux/bitfield.h>

#define PMU_ADDR 0x60
#define PMU_DATA 0x64

#define PMU5_MAINPLL_CPU 1
#define PMU5_MAINPLL_MEM	2
#define PMU5_MAINPLL_SSB	3

#define PPL_P1P2_PMU_REG		0
#define PPL_P1_MASK		0x0f000000
#define PPL_P2_MASK		0x00f00000
#define PPL_M14_OFF		1
#define PPL_MDIV_MASK		0x000000ff
#define PPL_MDIV_WIDTH		8
#define PPL_NM5_OFF		2
#define PPL_NDIV_MASK		0xfff00000
#define PPL_FMAB_OFF		3
#define PPL_MRAT_MASK		0xf0000000
#define PPL_ABRAT_MASK		0x08000000
#define PPL_FDIV_MASK		0x07ffffff
#define PPL_PLLCTL_OFF		4
#define PPL_PCHI_OFF		5
#define PPL_PCHI_MASK		0x0000003f

struct pmu_clk_info {
	int base_freq;
	int pll_reg;
};

struct pmu_clk_priv {
	void __iomem *regs;
	struct clk_hw *alp_clk;
	struct clk_hw *cpu_clk;
};

static u32 pmu_read(struct pmu_clk_priv *priv, int offset)
{
	writel(offset, priv->regs + PMU_ADDR);
	readl(priv->regs + PMU_ADDR);
	return readl(priv->regs + PMU_DATA);
}

static int pmu_cpu_freq(const struct pmu_clk_info *info, struct pmu_clk_priv *priv)
{
	u32 pll0 = info->pll_reg;
	u32 m = PMU5_MAINPLL_SSB;
	u32 div;
	u32 ndiv;
	u32 p1;
	u32 p2;
	u32 fc;
	u32 tmp;

	tmp = pmu_read(priv, pll0 + PPL_P1P2_PMU_REG);
	p1 = FIELD_GET(PPL_P1_MASK, tmp);
	p2 = FIELD_GET(PPL_P2_MASK, tmp);

	tmp = pmu_read(priv, pll0 + PPL_M14_OFF);
	div = (tmp >> ((m - 1) * PPL_MDIV_WIDTH)) &
		PPL_MDIV_WIDTH;

	tmp = pmu_read(priv, pll0 + PPL_NM5_OFF);
	ndiv = FIELD_GET(PPL_NDIV_MASK, tmp);

	fc = info->base_freq;
	fc = (p1 * ndiv * fc) / p2;
	fc = fc / div;

	return fc;
}

static int bcm_cc_pmu_probe(struct platform_device *pdev)
{
	struct pmu_clk_priv *priv;
	struct device *dev = &pdev->dev;
	const struct pmu_clk_info *pmu;
	int cpu_freq = 0;

	priv = devm_kzalloc(dev, sizeof(*priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;
	platform_set_drvdata(pdev, priv);

	priv->regs = devm_platform_ioremap_resource(pdev, 0);
	if (IS_ERR(priv->regs))
		return PTR_ERR(priv->regs);

	pmu = of_device_get_match_data(dev);

	priv->alp_clk = clk_hw_register_fixed_rate(dev, "alp-test", NULL, 0, pmu->base_freq);
	if (!priv->alp_clk)
		return -ENOMEM;

	cpu_freq = pmu_cpu_freq(pmu, priv);

	priv->cpu_clk = clk_hw_register_fixed_rate(dev, "cpu-test", NULL, 0, cpu_freq);
	if (!priv->cpu_clk)
		return -ENOMEM;

	pr_info("PMU clocks registered\n");

	return 0;
}

static const struct pmu_clk_info bcm4718_pmu_info = {
	.base_freq = 20000000,
	.pll_reg = 12,
};

static const struct of_device_id bcm_cc_pmu_of_match[] = {
	{ .compatible = "brcm,bcm4718-cc-pmu", .data = &bcm4718_pmu_info },
	{}
};
MODULE_DEVICE_TABLE(of, bcm_cc_pmu_of_match);

static struct platform_driver bcm_cc_pmu_driver = {
	.driver = {
		.name = "bcm-cc-pmu",
		.of_match_table = bcm_cc_pmu_of_match,
	},
	.probe          = bcm_cc_pmu_probe,
};

builtin_platform_driver(bcm_cc_pmu_driver);
