`timescale 1ns / 1ps

module tb_pl_vec_add_axi;
    reg         clk = 0;
    reg         rstn = 0;
    reg  [4:0]  awaddr = 0;
    reg         awvalid = 0;
    wire        awready;
    reg  [31:0] wdata = 0;
    reg  [3:0]  wstrb = 4'hF;
    reg         wvalid = 0;
    wire        wready;
    wire [1:0]  bresp;
    wire        bvalid;
    reg         bready = 0;
    reg  [4:0]  araddr = 0;
    reg         arvalid = 0;
    wire        arready;
    wire [31:0] rdata;
    wire [1:0]  rresp;
    wire        rvalid;
    reg         rready = 0;

    pl_vec_add_axi dut (
        .s_axi_aclk(clk), .s_axi_aresetn(rstn),
        .s_axi_awaddr(awaddr), .s_axi_awprot(3'b000), .s_axi_awvalid(awvalid), .s_axi_awready(awready),
        .s_axi_wdata(wdata), .s_axi_wstrb(wstrb), .s_axi_wvalid(wvalid), .s_axi_wready(wready),
        .s_axi_bresp(bresp), .s_axi_bvalid(bvalid), .s_axi_bready(bready),
        .s_axi_araddr(araddr), .s_axi_arprot(3'b000), .s_axi_arvalid(arvalid), .s_axi_arready(arready),
        .s_axi_rdata(rdata), .s_axi_rresp(rresp), .s_axi_rvalid(rvalid), .s_axi_rready(rready)
    );

    always #5 clk = ~clk; // 100 MHz Zynq FCLK_CLK0

    task axi_write(input [4:0] addr, input [31:0] data);
    begin
        @(posedge clk);
        awaddr <= addr; awvalid <= 1'b1;
        wdata  <= data; wvalid  <= 1'b1;
        bready <= 1'b1;
        wait (awready && wready);
        @(posedge clk);
        awvalid <= 1'b0; wvalid <= 1'b0;
        wait (bvalid);
        @(posedge clk);
        bready <= 1'b0;
    end
    endtask

    task axi_read(input [4:0] addr, output [31:0] data_out);
    begin
        @(posedge clk);
        araddr <= addr; arvalid <= 1'b1; rready <= 1'b1;
        wait (arready);
        @(posedge clk);
        arvalid <= 1'b0;
        wait (rvalid);
        data_out = rdata;
        @(posedge clk);
        rready <= 1'b0;
    end
    endtask

    task test_vector(input signed [31:0] a, input signed [31:0] b, input signed [31:0] expected);
        reg [31:0] status;
        reg signed [31:0] res;
    begin
        axi_write(5'h08, a);        // Write OPERAND_A
        axi_write(5'h0C, b);        // Write OPERAND_B
        axi_write(5'h00, 32'd1);    // CTRL = 1 (Start)
        axi_read(5'h04, status);    // Poll STATUS
        axi_read(5'h10, res);       // Read RESULT
        axi_write(5'h00, 32'd0);    // CTRL = 0 (Reset done)
        if (status[0] === 1'b1 && res === expected)
            $display("[PASS] AXI4-Lite PL Compute: %0d + %0d = %0d (STATUS=0x%0h)", a, b, res, status);
        else begin
            $display("[FAIL] Expected %0d, got %0d (STATUS=0x%0h)", expected, res, status);
            $finish;
        end
    end
    endtask

    initial begin
        $dumpfile("pl_vec_add.vcd");
        $dumpvars(0, tb_pl_vec_add_axi);
        #25 rstn = 1'b1;
        $display("=== Zynq PL AXI4-Lite Vector Accelerator RTL Simulation ===");
        test_vector(1, 10, 11);
        test_vector(16, 160, 176);
        test_vector(1024, 10240, 11264);
        test_vector(-50, 125, 75);
        $display("=== [SUCCESS] All AXI4-Lite PL Accelerator tests passed! ===");
        $finish;
    end
endmodule
