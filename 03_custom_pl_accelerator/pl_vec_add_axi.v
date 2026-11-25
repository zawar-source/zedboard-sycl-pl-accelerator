`timescale 1ns / 1ps

module pl_vec_add_axi #
(
    parameter integer C_S_AXI_DATA_WIDTH = 32,
    parameter integer C_S_AXI_ADDR_WIDTH = 5
)
(
    input  wire                                  s_axi_aclk,
    input  wire                                  s_axi_aresetn,
    input  wire [C_S_AXI_ADDR_WIDTH-1 : 0]       s_axi_awaddr,
    input  wire [2 : 0]                          s_axi_awprot,
    input  wire                                  s_axi_awvalid,
    output wire                                  s_axi_awready,
    input  wire [C_S_AXI_DATA_WIDTH-1 : 0]       s_axi_wdata,
    input  wire [(C_S_AXI_DATA_WIDTH/8)-1 : 0]   s_axi_wstrb,
    input  wire                                  s_axi_wvalid,
    output wire                                  s_axi_wready,
    output wire [1 : 0]                          s_axi_bresp,
    output wire                                  s_axi_bvalid,
    input  wire                                  s_axi_bready,
    input  wire [C_S_AXI_ADDR_WIDTH-1 : 0]       s_axi_araddr,
    input  wire [2 : 0]                          s_axi_arprot,
    input  wire                                  s_axi_arvalid,
    output wire                                  s_axi_arready,
    output wire [C_S_AXI_DATA_WIDTH-1 : 0]       s_axi_rdata,
    output wire [1 : 0]                          s_axi_rresp,
    output wire                                  s_axi_rvalid,
    input  wire                                  s_axi_rready
);

    reg [C_S_AXI_ADDR_WIDTH-1 : 0] axi_awaddr;
    reg                            axi_awready;
    reg                            axi_wready;
    reg [1 : 0]                    axi_bresp;
    reg                            axi_bvalid;
    reg [C_S_AXI_ADDR_WIDTH-1 : 0] axi_araddr;
    reg                            axi_arready;
    reg [C_S_AXI_DATA_WIDTH-1 : 0] axi_rdata;
    reg [1 : 0]                    axi_rresp;
    reg                            axi_rvalid;
    reg                            aw_en;

    // User Registers: 0x00=CTRL, 0x04=STATUS, 0x08=REG_A, 0x0C=REG_B, 0x10=RESULT
    reg [31:0] reg_ctrl;
    reg [31:0] reg_a;
    reg [31:0] reg_b;
    reg [31:0] reg_result;
    reg        done_flag;

    wire slv_reg_wren;
    wire slv_reg_rden;
    integer byte_index;

    assign s_axi_awready = axi_awready;
    assign s_axi_wready  = axi_wready;
    assign s_axi_bresp   = axi_bresp;
    assign s_axi_bvalid  = axi_bvalid;
    assign s_axi_arready = axi_arready;
    assign s_axi_rdata   = axi_rdata;
    assign s_axi_rresp   = axi_rresp;
    assign s_axi_rvalid  = axi_rvalid;

    // Write Address Ready
    always @(posedge s_axi_aclk) begin
        if (!s_axi_aresetn) begin
            axi_awready <= 1'b0;
            aw_en       <= 1'b1;
        end else if (~axi_awready && s_axi_awvalid && s_axi_wvalid && aw_en) begin
            axi_awready <= 1'b1;
            aw_en       <= 1'b0;
        end else if (s_axi_bready && axi_bvalid) begin
            aw_en       <= 1'b1;
            axi_awready <= 1'b0;
        end else begin
            axi_awready <= 1'b0;
        end
    end

    // Latch Write Address
    always @(posedge s_axi_aclk) begin
        if (!s_axi_aresetn)
            axi_awaddr <= 0;
        else if (~axi_awready && s_axi_awvalid && s_axi_wvalid && aw_en)
            axi_awaddr <= s_axi_awaddr;
    end

    // Write Data Ready
    always @(posedge s_axi_aclk) begin
        if (!s_axi_aresetn)
            axi_wready <= 1'b0;
        else if (~axi_wready && s_axi_wvalid && s_axi_awvalid && aw_en)
            axi_wready <= 1'b1;
        else
            axi_wready <= 1'b0;
    end

    assign slv_reg_wren = axi_wready && s_axi_wvalid && axi_awready && s_axi_awvalid;

    // Register Write Logic
    always @(posedge s_axi_aclk) begin
        if (!s_axi_aresetn) begin
            reg_ctrl <= 32'd0;
            reg_a    <= 32'd0;
            reg_b    <= 32'd0;
        end else if (slv_reg_wren) begin
            case (axi_awaddr[4:2])
                3'b000: begin // 0x00: CTRL
                    for (byte_index = 0; byte_index <= 3; byte_index = byte_index + 1)
                        if (s_axi_wstrb[byte_index])
                            reg_ctrl[(byte_index*8) +: 8] <= s_axi_wdata[(byte_index*8) +: 8];
                end
                3'b010: begin // 0x08: OPERAND_A
                    for (byte_index = 0; byte_index <= 3; byte_index = byte_index + 1)
                        if (s_axi_wstrb[byte_index])
                            reg_a[(byte_index*8) +: 8] <= s_axi_wdata[(byte_index*8) +: 8];
                end
                3'b011: begin // 0x0C: OPERAND_B
                    for (byte_index = 0; byte_index <= 3; byte_index = byte_index + 1)
                        if (s_axi_wstrb[byte_index])
                            reg_b[(byte_index*8) +: 8] <= s_axi_wdata[(byte_index*8) +: 8];
                end
                default: ;
            endcase
        end
    end

    // Hardware Vector Addition Execution Engine
    always @(posedge s_axi_aclk) begin
        if (!s_axi_aresetn) begin
            reg_result <= 32'd0;
            done_flag  <= 1'b0;
        end else if (!reg_ctrl[0]) begin
            done_flag  <= 1'b0;
        end else if (reg_ctrl[0] && !done_flag) begin
            reg_result <= $signed(reg_a) + $signed(reg_b);
            done_flag  <= 1'b1;
        end
    end

    // Write Response
    always @(posedge s_axi_aclk) begin
        if (!s_axi_aresetn) begin
            axi_bvalid <= 1'b0;
            axi_bresp  <= 2'b0;
        end else if (axi_awready && s_axi_awvalid && ~axi_bvalid && axi_wready && s_axi_wvalid) begin
            axi_bvalid <= 1'b1;
            axi_bresp  <= 2'b0;
        end else if (s_axi_bready && axi_bvalid) begin
            axi_bvalid <= 1'b0;
        end
    end

    // Read Address Ready
    always @(posedge s_axi_aclk) begin
        if (!s_axi_aresetn) begin
            axi_arready <= 1'b0;
            axi_araddr  <= 0;
        end else if (~axi_arready && s_axi_arvalid) begin
            axi_arready <= 1'b1;
            axi_araddr  <= s_axi_araddr;
        end else begin
            axi_arready <= 1'b0;
        end
    end

    // Read Data Valid
    always @(posedge s_axi_aclk) begin
        if (!s_axi_aresetn) begin
            axi_rvalid <= 1'b0;
            axi_rresp  <= 2'b0;
        end else if (axi_arready && s_axi_arvalid && ~axi_rvalid) begin
            axi_rvalid <= 1'b1;
            axi_rresp  <= 2'b0;
        end else if (axi_rvalid && s_axi_rready) begin
            axi_rvalid <= 1'b0;
        end
    end

    assign slv_reg_rden = axi_arready & s_axi_arvalid & ~axi_rvalid;

    // Register Read Mux
    always @(posedge s_axi_aclk) begin
        if (!s_axi_aresetn) begin
            axi_rdata <= 32'd0;
        end else if (slv_reg_rden) begin
            case (axi_araddr[4:2])
                3'b000:  axi_rdata <= reg_ctrl;
                3'b001:  axi_rdata <= {31'd0, done_flag}; // STATUS
                3'b010:  axi_rdata <= reg_a;
                3'b011:  axi_rdata <= reg_b;
                3'b100:  axi_rdata <= reg_result;           // RESULT
                default: axi_rdata <= 32'd0;
            endcase
        end
    end

endmodule
