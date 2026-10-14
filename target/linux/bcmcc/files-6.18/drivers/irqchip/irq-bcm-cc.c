// SPDX-License-Identifier: GPL-2.0-only

/* TODO: support multiple parent irq. Put everything on irq 2 for now */

#include <linux/of_irq.h>
#include <linux/irqchip.h>
#include <linux/spinlock.h>
#include <linux/of_address.h>
#include <linux/irqchip/chained_irq.h>

#define BCMA_MIPS_MIPS74K_INTMASK_INT0	0x0014

static DEFINE_RAW_SPINLOCK(irq_lock);
static void __iomem *bcma_mips_base, *cc_irq;


struct cc_core_output {
	struct irq_domain	*domain;
	u32			mask;
};

static void bcma_mask_irq(struct irq_data *i)
{
	unsigned long flags;
	u32 mask;

	raw_spin_lock_irqsave(&irq_lock, flags);

	mask = readl(bcma_mips_base + BCMA_MIPS_MIPS74K_INTMASK_INT0);
	mask &= ~BIT(i->hwirq);
	writel(mask, bcma_mips_base + BCMA_MIPS_MIPS74K_INTMASK_INT0);


	raw_spin_unlock_irqrestore(&irq_lock, flags);
}

static void bcma_unmask_irq(struct irq_data *i)
{
	unsigned long flags;
	u32 mask;

	raw_spin_lock_irqsave(&irq_lock, flags);


	mask = readl(bcma_mips_base + BCMA_MIPS_MIPS74K_INTMASK_INT0);
	mask |= BIT(i->hwirq);
	writel(mask, bcma_mips_base + BCMA_MIPS_MIPS74K_INTMASK_INT0);

	raw_spin_unlock_irqrestore(&irq_lock, flags);
}

static struct irq_chip bcma_irq = {
	.name = "bcma-cc-intc",
	.irq_mask = bcma_mask_irq,
	.irq_unmask = bcma_unmask_irq,
};

static int intc_map(struct irq_domain *d, unsigned int irq, irq_hw_number_t hw)
{
	irq_set_chip_and_handler(irq, &bcma_irq, handle_level_irq);

	pr_err("Mapping IRQ %d to %lu\n", irq, hw);

	return 0;
}

static const struct irq_domain_ops irq_domain_ops = {
	.map = intc_map,
	.xlate = irq_domain_xlate_onecell,
};

static void bcma_irq_dispatch(struct irq_desc *desc)
{
	struct irq_chip *chip = irq_desc_get_chip(desc);
	struct irq_domain *domain;
	unsigned long pending;
	unsigned int soc_int;

	chained_irq_enter(chip, desc);
	pending = readl(bcma_mips_base + BCMA_MIPS_MIPS74K_INTMASK_INT0);

	if (unlikely(!pending)) {
		spurious_interrupt();
		goto out;
	}

	domain = irq_desc_get_handler_data(desc);
	for_each_set_bit(soc_int, &pending, 32)
		generic_handle_domain_irq(domain, soc_int);

out:
	chained_irq_exit(chip, desc);
}


static int __init bcma_irq_of_init(struct device_node *node, struct device_node *parent)
{
	struct irq_domain *domain;
	int parent_irq;

	bcma_mips_base = of_iomap(node, 0);
	if (!bcma_mips_base)
		return -ENXIO;

	cc_irq = of_iomap(node, 1);
	if (!cc_irq)
		return -ENXIO;

	parent_irq = of_irq_get(node, 0);

	if (parent_irq < 0)
		return parent_irq;
	else if (!parent_irq)
		return -ENODEV;

	domain = irq_domain_create_linear(of_fwnode_handle(node), 32, &irq_domain_ops, NULL);
	if (!domain)
		return -ENOMEM;

	irq_set_chained_handler_and_data(parent_irq, bcma_irq_dispatch, domain);

	/* Is this necessary? */
	write_c0_count(0);
	write_c0_compare(0xffff);

	/* Writing bit 31 in the irq 5 register didn't do anything, so keep it on 7
	 * like the vendor code does */
	writel(1 << 31, bcma_mips_base + BCMA_MIPS_MIPS74K_INTMASK_INT0 + 0x14);
	cp0_compare_irq = 7;

	return 0;
}

IRQCHIP_DECLARE(bcma_cc_intc, "brcm,bcma-cc-intc", bcma_irq_of_init);
