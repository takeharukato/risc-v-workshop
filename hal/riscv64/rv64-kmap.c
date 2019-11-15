/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  OS kernel sample                                                  */
/*  Copyright 2019 Takeharu KATO                                      */
/*                                                                    */
/*  RISC-V64 spinlock routines                                        */
/*                                                                    */
/**********************************************************************/

#include <klib/freestanding.h>
#include <kern/kern-common.h>
#include <hal/rv64-platform.h>
#include <hal/hal-pgtbl.h>

uint64_t kern_upper_vpn1_tbl[RV64_PGTBL_ENTRIES_NR*RV64_BOOT_PGTBL_VPN1_NR] __attribute__ ((aligned (4096))) ;
uint64_t kern_lower_vpn1_tbl[RV64_PGTBL_ENTRIES_NR*RV64_BOOT_PGTBL_VPN1_NR] __attribute__ ((aligned (4096))) ;
uint64_t kern_vpn2_tbl[RV64_PGTBL_ENTRIES_NR] __attribute__ ((aligned (4096))) ;

#define RV64_KERN_PTE_FLAGS (RV64_PTE_R|RV64_PTE_W|RV64_PTE_A|RV64_PTE_D)

void
rv64_show_pte(int lvl, int idx, vm_paddr phy_ptep, uint64_t pte){

	kprintf("vpn%d[%d]=%p paddr=%p flags=[%c|%c|%c|%c|%c|%c|%c|%c]\n",
		lvl, idx, phy_ptep, RV64_PTE_TO_PADDR(pte),
		( ( RV64_PTE_FLAGS(pte) & RV64_PTE_V ) ? ('V') : ('-') ),
		( ( RV64_PTE_FLAGS(pte) & RV64_PTE_R ) ? ('R') : ('-') ),
		( ( RV64_PTE_FLAGS(pte) & RV64_PTE_W ) ? ('W') : ('-') ),
		( ( RV64_PTE_FLAGS(pte) & RV64_PTE_X ) ? ('X') : ('-') ),
		( ( RV64_PTE_FLAGS(pte) & RV64_PTE_U ) ? ('U') : ('-') ),
		( ( RV64_PTE_FLAGS(pte) & RV64_PTE_G ) ? ('G') : ('-') ),
		( ( RV64_PTE_FLAGS(pte) & RV64_PTE_A ) ? ('A') : ('-') ),
		( ( RV64_PTE_FLAGS(pte) & RV64_PTE_D ) ? ('D') : ('-') ));
}

void
rv64_show_page_table(void *kv_pgtbl_base){
	uint64_t *vpn0_tbl;
	uint64_t *vpn1_tbl;
	uint64_t *vpn2_tbl;
	int          i,j,k;

	vpn2_tbl = kv_pgtbl_base;
	for(i = 0; RV64_PGTBL_ENTRIES_NR > i; ++i) {
		
		if ( RV64_PTE_FLAGS(vpn2_tbl[i]) & RV64_PTE_V ) {
			
			rv64_show_pte(2, i, HAL_KERN_STRAIGHT_TO_PHY(vpn2_tbl), vpn2_tbl[i]);
			if ( ( RV64_PTE_FLAGS(vpn2_tbl[i]) & (RV64_PTE_R|RV64_PTE_X) ) ==
			     (RV64_PTE_R|RV64_PTE_X) )
				continue;
			vpn1_tbl = (uint64_t *)
				HAL_PHY_TO_KERN_STRAIGHT(RV64_PTE_TO_PADDR(vpn2_tbl[i]));
			for(j = 0; RV64_PGTBL_ENTRIES_NR > j; ++j) {

				if ( RV64_PTE_FLAGS(vpn1_tbl[j]) & RV64_PTE_V ) {

					rv64_show_pte(1, j, HAL_KERN_STRAIGHT_TO_PHY(vpn1_tbl), 
						      vpn1_tbl[j]);
					if ( ( RV64_PTE_FLAGS(vpn1_tbl[i]) & (RV64_PTE_R|RV64_PTE_X) ) == (RV64_PTE_R|RV64_PTE_X) )
						continue;
					vpn0_tbl = (uint64_t *)
						HAL_PHY_TO_KERN_STRAIGHT(
							RV64_PTE_TO_PADDR(vpn1_tbl[j]));
					for(k = 0; RV64_PGTBL_ENTRIES_NR > k; ++k) {

						if ( RV64_PTE_FLAGS(vpn0_tbl[k]) & RV64_PTE_V ) {
							rv64_show_pte(0, k, HAL_KERN_STRAIGHT_TO_PHY(vpn1_tbl), vpn0_tbl[k]);
						}
					}
				}
			}
		}
	}
}
/**
   カーネル空間をマップする
 */
int
rv64_map_kernel_space(void){
	int          vpn1idx;
	int    upper_vpn2idx;
	int    lower_vpn2idx;
	vm_paddr       paddr;

	for( paddr = HAL_KERN_PHY_BASE, vpn1idx = 0; RV64_STRAIGHT_MAPSIZE > paddr;
	     paddr += HAL_PAGE_SIZE_2M, ++vpn1idx) {

		upper_vpn2idx = rv64_pgtbl_vpn2_index(HAL_PHY_TO_KERN_STRAIGHT(paddr));
		lower_vpn2idx = rv64_pgtbl_vpn2_index(paddr);

		kern_upper_vpn1_tbl[vpn1idx] =
			RV64_PTE_V|RV64_PTE_X|RV64_KERN_PTE_FLAGS|RV64_PTE_2MPADDR_TO_PPN(paddr);
		kern_lower_vpn1_tbl[vpn1idx] =
			RV64_PTE_V|RV64_PTE_X|RV64_KERN_PTE_FLAGS|RV64_PTE_2MPADDR_TO_PPN(paddr);

		if ( vpn1idx % RV64_PGTBL_ENTRIES_NR == 0 ) {

			kern_vpn2_tbl[upper_vpn2idx] = RV64_PTE_V|RV64_KERN_PTE_FLAGS|
				RV64_PTE_PADDR_TO_PPN(HAL_KERN_STRAIGHT_TO_PHY(
							      (void *)&kern_upper_vpn1_tbl[0] + (4096 * vpn1idx/RV64_PGTBL_ENTRIES_NR)));	

			kern_vpn2_tbl[lower_vpn2idx] = RV64_PTE_V|RV64_KERN_PTE_FLAGS|
						      RV64_PTE_PADDR_TO_PPN(HAL_KERN_STRAIGHT_TO_PHY((void *)&kern_lower_vpn1_tbl[0] + (4096 * vpn1idx/RV64_PGTBL_ENTRIES_NR)));	

		}
	}
	rv64_show_page_table(&kern_vpn2_tbl[0]);

	return 0;
}
